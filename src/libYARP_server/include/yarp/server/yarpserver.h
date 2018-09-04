#ifndef YARPSERVERLIBHDR
#define YARPSERVERLIBHDR

#include <yarp/os/Port.h>
#include <yarp/os/Contact.h>
#include <yarp/os/network.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <yarp/server/api.h>

namespace yarp
{
namespace server
{


//network protocol

constexpr char registerCmd[]   = "register";
constexpr char unregisterCmd[] = "unregister";
constexpr char queryCmd[]      = "query";
constexpr char listCmd[]       = "list";
constexpr char runnersCmd[]    = "runners";
constexpr char setCmd[]        = "set";
constexpr char getCmd[]        = "get";
constexpr char checkCmd[]      = "check";
constexpr char routeCmd[]      = "route";
constexpr char gcCmd[]         = "gc";
constexpr char helpCmd[]       = "help";

class YARP_server_API NameServer
{
    yarp::os::Network yarpnetwork;
    yarp::os::Port    cmdReceiver;
    bool              mustStop{false};

public:
    void start();
    void stop() { mustStop = true; };
};



}
};

#endif //header guard