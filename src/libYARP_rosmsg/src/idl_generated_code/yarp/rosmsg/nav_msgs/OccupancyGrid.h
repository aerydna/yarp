/*
 * Copyright (C) 2006-2021 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

// This is an automatically generated file.

// Generated from the following "nav_msgs/OccupancyGrid" msg definition:
//   # This represents a 2-D grid map, in which each cell represents the probability of
//   # occupancy.
//   
//   Header header 
//   
//   #MetaData for the map
//   MapMetaData info
//   
//   # The map data, in row-major order, starting with (0,0).  Occupancy
//   # probabilities are in the range [0,100].  Unknown is -1.
//   int8[] data
// Instances of this class can be read and written with YARP ports,
// using a ROS-compatible format.

#ifndef YARP_ROSMSG_nav_msgs_OccupancyGrid_h
#define YARP_ROSMSG_nav_msgs_OccupancyGrid_h

#include <yarp/os/Wire.h>
#include <yarp/os/Type.h>
#include <yarp/os/idl/WireTypes.h>
#include <string>
#include <vector>
#include <yarp/rosmsg/std_msgs/Header.h>
#include <yarp/rosmsg/nav_msgs/MapMetaData.h>

namespace yarp {
namespace rosmsg {
namespace nav_msgs {

class OccupancyGrid : public yarp::os::idl::WirePortable
{
public:
    yarp::rosmsg::std_msgs::Header header;
    yarp::rosmsg::nav_msgs::MapMetaData info;
    std::vector<std::int8_t> data;

    OccupancyGrid() :
            header(),
            info(),
            data()
    {
    }

    void clear()
    {
        // *** header ***
        header.clear();

        // *** info ***
        info.clear();

        // *** data ***
        data.clear();
    }

    bool readBare(yarp::os::ConnectionReader& connection) override
    {
        // *** header ***
        if (!header.read(connection)) {
            return false;
        }

        // *** info ***
        if (!info.read(connection)) {
            return false;
        }

        // *** data ***
        int len = connection.expectInt32();
        data.resize(len);
        if (len > 0 && !connection.expectBlock((char*)&data[0], sizeof(std::int8_t)*len)) {
            return false;
        }

        return !connection.isError();
    }

    bool readBottle(yarp::os::ConnectionReader& connection) override
    {
        connection.convertTextMode();
        yarp::os::idl::WireReader reader(connection);
        if (!reader.readListHeader(3)) {
            return false;
        }

        // *** header ***
        if (!header.read(connection)) {
            return false;
        }

        // *** info ***
        if (!info.read(connection)) {
            return false;
        }

        // *** data ***
        if (connection.expectInt32() != (BOTTLE_TAG_LIST|BOTTLE_TAG_INT8)) {
            return false;
        }
        int len = connection.expectInt32();
        data.resize(len);
        for (int i=0; i<len; i++) {
            data[i] = (std::int8_t)connection.expectInt8();
        }

        return !connection.isError();
    }

    using yarp::os::idl::WirePortable::read;
    bool read(yarp::os::ConnectionReader& connection) override
    {
        return (connection.isBareMode() ? readBare(connection)
                                        : readBottle(connection));
    }

    bool writeBare(yarp::os::ConnectionWriter& connection) const override
    {
        // *** header ***
        if (!header.write(connection)) {
            return false;
        }

        // *** info ***
        if (!info.write(connection)) {
            return false;
        }

        // *** data ***
        connection.appendInt32(data.size());
        if (data.size()>0) {
            connection.appendExternalBlock((char*)&data[0], sizeof(std::int8_t)*data.size());
        }

        return !connection.isError();
    }

    bool writeBottle(yarp::os::ConnectionWriter& connection) const override
    {
        connection.appendInt32(BOTTLE_TAG_LIST);
        connection.appendInt32(3);

        // *** header ***
        if (!header.write(connection)) {
            return false;
        }

        // *** info ***
        if (!info.write(connection)) {
            return false;
        }

        // *** data ***
        connection.appendInt32(BOTTLE_TAG_LIST|BOTTLE_TAG_INT8);
        connection.appendInt32(data.size());
        for (size_t i=0; i<data.size(); i++) {
            connection.appendInt8(data[i]);
        }

        connection.convertTextMode();
        return !connection.isError();
    }

    using yarp::os::idl::WirePortable::write;
    bool write(yarp::os::ConnectionWriter& connection) const override
    {
        return (connection.isBareMode() ? writeBare(connection)
                                        : writeBottle(connection));
    }

    // This class will serialize ROS style or YARP style depending on protocol.
    // If you need to force a serialization style, use one of these classes:
    typedef yarp::os::idl::BareStyle<yarp::rosmsg::nav_msgs::OccupancyGrid> rosStyle;
    typedef yarp::os::idl::BottleStyle<yarp::rosmsg::nav_msgs::OccupancyGrid> bottleStyle;

    // The name for this message, ROS will need this
    static constexpr const char* typeName = "nav_msgs/OccupancyGrid";

    // The checksum for this message, ROS will need this
    static constexpr const char* typeChecksum = "3381f2d731d4076ec5c71b0759edbe4e";

    // The source text for this message, ROS will need this
    static constexpr const char* typeText = "\
# This represents a 2-D grid map, in which each cell represents the probability of\n\
# occupancy.\n\
\n\
Header header \n\
\n\
#MetaData for the map\n\
MapMetaData info\n\
\n\
# The map data, in row-major order, starting with (0,0).  Occupancy\n\
# probabilities are in the range [0,100].  Unknown is -1.\n\
int8[] data\n\
\n\
================================================================================\n\
MSG: std_msgs/Header\n\
# Standard metadata for higher-level stamped data types.\n\
# This is generally used to communicate timestamped data \n\
# in a particular coordinate frame.\n\
# \n\
# sequence ID: consecutively increasing ID \n\
uint32 seq\n\
#Two-integer timestamp that is expressed as:\n\
# * stamp.sec: seconds (stamp_secs) since epoch (in Python the variable is called 'secs')\n\
# * stamp.nsec: nanoseconds since stamp_secs (in Python the variable is called 'nsecs')\n\
# time-handling sugar is provided by the client library\n\
time stamp\n\
#Frame this data is associated with\n\
# 0: no frame\n\
# 1: global frame\n\
string frame_id\n\
\n\
================================================================================\n\
MSG: nav_msgs/MapMetaData\n\
# This hold basic information about the characterists of the OccupancyGrid\n\
\n\
# The time at which the map was loaded\n\
time map_load_time\n\
# The map resolution [m/cell]\n\
float32 resolution\n\
# Map width [cells]\n\
uint32 width\n\
# Map height [cells]\n\
uint32 height\n\
# The origin of the map [m, m, rad].  This is the real-world pose of the\n\
# cell (0,0) in the map.\n\
geometry_msgs/Pose origin\n\
================================================================================\n\
MSG: geometry_msgs/Pose\n\
# A representation of pose in free space, composed of position and orientation. \n\
Point position\n\
Quaternion orientation\n\
\n\
================================================================================\n\
MSG: geometry_msgs/Point\n\
# This contains the position of a point in free space\n\
float64 x\n\
float64 y\n\
float64 z\n\
\n\
================================================================================\n\
MSG: geometry_msgs/Quaternion\n\
# This represents an orientation in free space in quaternion form.\n\
\n\
float64 x\n\
float64 y\n\
float64 z\n\
float64 w\n\
";

    yarp::os::Type getType() const override
    {
        yarp::os::Type typ = yarp::os::Type::byName(typeName, typeName);
        typ.addProperty("md5sum", yarp::os::Value(typeChecksum));
        typ.addProperty("message_definition", yarp::os::Value(typeText));
        return typ;
    }
};

} // namespace nav_msgs
} // namespace rosmsg
} // namespace yarp

#endif // YARP_ROSMSG_nav_msgs_OccupancyGrid_h
