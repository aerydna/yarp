/*
 * Copyright (C) 2006-2021 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InputCallback.h"
#include "TextureBuffer.h"
#include "OVRHeadsetLogComponent.h"

#include <yarp/os/BufferedPort.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Time.h>
#include <yarp/os/Value.h>

InputCallback::InputCallback(int eye) :
        yarp::os::BufferedPort<ImageType>(),
        eyeRenderTexture(nullptr),
        eye(eye),
        expected(0),
        droppedFrames(0),
        lastImageWidth(0),
        lastImageHeight(0),
        rollOffset(0.0f),
        pitchOffset(0.0f),
        yawOffset(0.0f)
{
    yCTrace(OVRHEADSET);
}


InputCallback::~InputCallback()
{
    yCTrace(OVRHEADSET);

    if (eyeRenderTexture) {
        delete eyeRenderTexture;
    }
    eyeRenderTexture = nullptr;
}


void InputCallback::onRead(ImageType &img)
{
    int delaycnt = 0;

    eyeRenderTexture->mutex.lock();
    while (eyeRenderTexture->dataReady && delaycnt <= 3) {
        eyeRenderTexture->mutex.unlock();
        yarp::os::SystemClock::delaySystem(0.001);
        eyeRenderTexture->mutex.lock();
        ++delaycnt;
    }

    if (eyeRenderTexture->dataReady) {
        ++droppedFrames;
    }

// Debug dropped frames using alternating 30x30 red and green squares
// at the bottom right of the image, see gazebo_yarp_plugins camera
// plugin.
#define DEBUG_SQUARES 0
#if DEBUG_SQUARES
    int found = -1;
    for (int i = 0; i < 10; ++i) {
        unsigned char* pix = img.getPixelAddress(img.width()-15-(i*30),img.height()-15);
        if(i % 2) {
            // Check if this is a RED pixel (255,0,0)
            // lossy carriers will change the value, so we look for a
            // pixel close enough to the one we are looking for
            if (pix[0] <= 5 && pix[1] >= 250 && pix[2] <= 5) {
                found = i;
                break;
            }
        } else {
            // Check if this is a GREEN pixel (0,255,0)
            if (pix[0] >= 250 && pix[1] <= 5 && pix[2] <= 5) {
                found = i;
                break;
            }
        }
    }
    if (found != expected) {
        yCWarning(OVRHEADSET) << "InputCallback" << (eye==0?"left ":"right") << "    expected" << expected << "found" << found << "next" <<  (found + 1) % 10;
    }
    expected = (found + 1) % 10;
#endif // DEBUG_SQUARES

    if(eyeRenderTexture->ptr) {
        size_t w = img.width();
        size_t h = img.height();
        size_t rs = img.getRowSize();
        unsigned char *data = img.getRawImage();

        // update data directly on the mapped buffer
        if (eyeRenderTexture->width == w && eyeRenderTexture->height == h) {
            // Texture and image have the same size: no problems
            memcpy(eyeRenderTexture->ptr, data, eyeRenderTexture->bufferSize);
        } else if (eyeRenderTexture->width >= w && eyeRenderTexture->height >= h) {
            // Texture is larger than image: image is centered in the texture
            int x = (eyeRenderTexture->width - w)/2;
            int y = (eyeRenderTexture->height - h)/2;
            for (size_t i = 0; i < h; ++i) {
                unsigned char* textureStart = eyeRenderTexture->ptr + (y+i)*eyeRenderTexture->rowSize + x*3;
                unsigned char* dataStart = data + (i*rs);
                memcpy(textureStart, dataStart, rs);
            }
        } else {
            // Texture is smaller than image: image is cropped
            int x = (w - eyeRenderTexture->width)/2;
            int y = (h - eyeRenderTexture->height)/2;
            for (size_t i = 0; i < eyeRenderTexture->width; ++i) {
                unsigned char* textureStart = eyeRenderTexture->ptr + (y+i)*(i*eyeRenderTexture->rowSize) + x*3;
                unsigned char* dataStart = data + (y+i)*rs + x*3;
                memcpy(textureStart, dataStart, eyeRenderTexture->rowSize);
            }
        }

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float roll = rollOffset;
        float pitch = pitchOffset;
        float yaw = yawOffset;

        int seqNum;
        double ts, r, p, yy;

        yarp::os::Bottle b;
        yarp::os::BufferedPort<ImageType>::getEnvelope(b);
        int ret = std::sscanf(b.toString().c_str(), "%d %lg %lg %lg %lg\n", &seqNum, &ts, &r, &p, &yy);
        if (ret == 5) {
            roll += OVR::DegreeToRad(static_cast<float>(r));
            pitch += OVR::DegreeToRad(static_cast<float>(p));
            yaw += OVR::DegreeToRad(static_cast<float>(yy));
        }

        eyeRenderTexture->eyePose.Orientation.w = (float)(- cos(roll/2) * cos(pitch/2) * cos(yaw/2) - sin(roll/2) * sin(pitch/2) * sin(yaw/2));
        eyeRenderTexture->eyePose.Orientation.x = (float)(- cos(roll/2) * sin(pitch/2) * cos(yaw/2) - sin(roll/2) * cos(pitch/2) * sin(yaw/2));
        eyeRenderTexture->eyePose.Orientation.y = (float)(- cos(roll/2) * cos(pitch/2) * sin(yaw/2) + sin(roll/2) * sin(pitch/2) * cos(yaw/2));
        eyeRenderTexture->eyePose.Orientation.z = (float)(- sin(roll/2) * cos(pitch/2) * cos(yaw/2) + cos(roll/2) * sin(pitch/2) * sin(yaw/2));

        eyeRenderTexture->eyePose.Position.x = x;
        eyeRenderTexture->eyePose.Position.y = y;
        eyeRenderTexture->eyePose.Position.z = z;

        eyeRenderTexture->imageWidth = img.width();
        eyeRenderTexture->imageHeight = img.height();

        eyeRenderTexture->dataReady = true;
    }
    eyeRenderTexture->mutex.unlock();
}
