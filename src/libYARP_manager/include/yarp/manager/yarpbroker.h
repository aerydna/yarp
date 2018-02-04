/*
 * Copyright (C) 2006-2019 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#ifndef YARP_MANAGER_YARPBROKER
#define YARP_MANAGER_YARPBROKER

#include <string>
#include <iostream>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <string>
#include <yarp/os/Semaphore.h>
#include <yarp/os/PeriodicThread.h>

// FIXME Do not use yarp/os/impl in .h files
#include <yarp/os/SystemInfo.h>
#include <yarp/os/SystemInfoSerializer.h>

#include <yarp/manager/broker.h>
#include <yarp/manager/primresource.h>
#include <yarp/logger/YarpLogger.h>

namespace yarp {
namespace manager {

struct LogPort : yarp::os::Port
{
    std::vector<yarp::yarpLogger::MessageEntry> log;
    struct CallBackBottle : public yarp::os::PortReader
    {
        std::vector<yarp::yarpLogger::MessageEntry>* container;
        virtual bool read(yarp::os::ConnectionReader& reader) override
        {
            yarp::os::Bottle b;
            bool             ret = b.read(reader);
            std::string      s(b.get(1).asString().c_str());

            if (container && b.size() == 2)
            {
                container->push_back(s);
            }
            return ret;
        }
    }logb;
    //yarp::os::Bottle log;
    LogPort()
    {
        logb.container = &log;
        this->setReader(logb);
    }

    bool connect(const std::string& remote)
    {
        int i = 0;
        while (!yarp::os::Network::exists(remote) && i < 5)
        {
            yarp::os::Time::delay(0.05);
            i++;
        }
        return yarp::os::Network::connect(remote, this->getName());
    }
};
/**
 * Class Broker
 */
class YarpBroker: public Broker, public yarp::os::PeriodicThread {

public:
    YarpBroker();
     virtual ~YarpBroker();
     bool init() override;
     bool init(const char* szcmd, const char* szparam,
            const char* szhost, const char* szstdio,
            const char* szworkdir, const char* szenv ) override;
     void fini(void) override;
     bool start() override;
     bool stop() override;
     bool kill() override;
     bool connect(const char* from, const char* to,
                        const char* carrier, bool persist=false) override;
     bool disconnect(const char* from, const char* to, const char* carrier) override;
     bool rmconnect(const char* from, const char* to);
     int running(void) override;
     bool exists(const char* port) override;
     const char* requestRpc(const char* szport, const char* request, double timeout) override;
     bool connected(const char* from, const char* to, const char* carrier) override;
     const char* error(void) override;
     bool initialized(void) override { return bInitialized;}
     bool attachStdout(void) override;
     void detachStdout(void) override;

     bool getSystemInfo(const char* server,
                        yarp::os::SystemInfoSerializer& info);
     bool getAllProcesses(const char* server,
                        ProcessContainer &processes);
     bool getAllPorts(std::vector<std::string> &stingList);

     bool setQos(const char* from, const char* to,
                 const char* qosFrom, const char* qosTo);

     std::vector<yarp::yarpLogger::MessageEntry>& getLog() { return log.log; }

public: // for rate thread
    void run() override;
    bool threadInit() override;
    void threadRelease() override;

protected:

private:
    std::string strCmd;
    std::string strParam;
    std::string strHost;
    std::string strStdio;
    std::string strWorkdir;
    std::string strTag;
    std::string strEnv;
    unsigned int ID;
    yarp::os::Property command;
    std::string strError;
    bool bOnlyConnector;
    bool bInitialized;
    yarp::os::Semaphore semParam;
    std::string strStdioUUID;
    std::string __trace_message;

    yarp::os::BufferedPort<yarp::os::Bottle> stdioPort;
    LogPort log;
    //yarp::os::Port port;

    bool timeout(double base, double timeout);
    yarp::os::Property& runProperty(void);
    int requestServer(yarp::os::Property& config);
    int SendMsg(yarp::os::Bottle& msg, std::string target,
                yarp::os::Bottle& resp, float fTimeout=5.0);
    bool getQosFromString(const char* qos, yarp::os::QosStyle& style);

};

} // namespace yarp
} // namespace manager


#endif // __YARP_MANAGER_YARPBROKER__
