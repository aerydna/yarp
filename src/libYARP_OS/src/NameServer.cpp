/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 * Copyright (C) 2006-2010 RobotCub Consortium
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/os/impl/NameServer.h>
#include <yarp/os/impl/Logger.h>
#include <yarp/os/ConnectionWriter.h>
#include <yarp/os/impl/SplitString.h>
#include <yarp/os/NetType.h>
#include <yarp/os/impl/NameConfig.h>
#include <yarp/os/ManagedBytes.h>
#include <yarp/os/impl/NameConfig.h>
#ifdef YARP_HAS_ACE
#  include <yarp/os/impl/FallbackNameServer.h>
#endif
#include <yarp/os/Time.h>
#include <yarp/os/Value.h>
#include <yarp/os/Port.h>
#include <yarp/os/Vocab.h>
#include <yarp/os/Network.h>
#include <yarp/os/Property.h>
#include <yarp/conf/version.h>

#include <map>
#include <set>

using namespace yarp::os::impl;
using namespace yarp::os;

//#define YMSG(x) printf x;
//#define YTRACE(x) YMSG(("at %s\n", x))

#define YMSG(x)
#define YTRACE(x)

// produce a correctly parsed string in presence of quoting
static std::string STR_HELP(const char *txt) {
    Value v;
    v.fromString(txt);
    return v.asString();
}
#define STR(x) STR_HELP(x).c_str()

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// Basic functionality
//
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


Contact NameServer::unregisterName(const std::string& name) {
    Contact prev = queryName(name);
    if (prev.isValid()) {
        if (prev.getPort()!=-1) {
            NameRecord& rec = getNameRecord(prev.getRegName());
            if (rec.isReusablePort()) {
                HostRecord& host = getHostRecord(prev.getHost());
                host.release(prev.getPort());
            }
            if (rec.isReusableIp()) {
                if (rec.getAddress().getCarrier()=="mcast") {
                    mcastRecord.releaseAddress(rec.getAddress().getHost().c_str());
                }
            }
            rec.clear();
            tmpNames.release(name);

            Bottle event;
            event.addVocab(Vocab::encode("del"));
            event.addString(name.c_str());
            onEvent(event);
        }
    }

    return queryName(name);
}



Contact NameServer::registerName(const std::string& name,
                                 const Contact& address,
                                 const std::string& remote) {
    bool reusablePort = false;
    bool reusableIp = false;

    //YARP_DEBUG(Logger::get(), "in registerName...");

    if (name!="...") {
        unregisterName(name);
    }

    Contact suggestion = address;

    if (!suggestion.isValid()) {
        suggestion = Contact(name, "...", "...", 0);
    }

    std::string portName = name;
    if (portName == "...") {
        portName = tmpNames.get();
    }

    std::string carrier = suggestion.getCarrier();
    if (carrier == "...") {
        carrier = "tcp";
    }

    std::string machine = suggestion.getHost();
    int overridePort = 0;
    if (machine == "...") {
        if (carrier!="mcast") {
            if (remote=="...") {
                YARP_ERROR(Logger::get(), "remote machine name was not found!  can only guess it is local...");
                machine = "127.0.0.1";
            } else {
                machine = remote;
            }
        } else {
            machine = mcastRecord.get();
            overridePort = mcastRecord.lastPortNumber();
            reusableIp = true;
        }
    }

    int port = suggestion.getPort();
    if (port == 0) {
        if (overridePort) {
            port = overridePort;
        } else {
            port = getHostRecord(machine).get();
            reusablePort = true;
        }
    }

    suggestion = Contact(portName, carrier, machine, port);

    YARP_DEBUG(Logger::get(), std::string("Registering ") +
               suggestion.toURI() + " for " + suggestion.getRegName());

    NameRecord& nameRecord = getNameRecord(suggestion.getRegName());
    nameRecord.setAddress(suggestion, reusablePort, reusableIp);

    Bottle event;
    event.addVocab(Vocab::encode("add"));
    event.addString(suggestion.getRegName().c_str());
    onEvent(event);

    return nameRecord.getAddress();
}


Contact NameServer::queryName(const std::string& name) {
    std::string base = name;
    std::string pat = "";
    if (name.find("/net=") == 0) {
        size_t patStart = 5;
        size_t patEnd = name.find('/', patStart);
        if (patEnd>=patStart && patEnd!=std::string::npos) {
            pat = name.substr(patStart, patEnd-patStart);
            base = name.substr(patEnd);
            YARP_DEBUG(Logger::get(), std::string("Special query form ") +
                       name + " (" + pat + "/" + base + ")");
        }
    }

    NameRecord *rec = getNameRecord(base, false);
    if (rec!=nullptr) {
        if (pat!="") {
            std::string ip = rec->matchProp("ips", pat);
            if (ip!="") {
                SplitString sip(ip.c_str());
                Contact c = rec->getAddress();
                c.setHost(sip.get(0));
                return c;
            }
        }
        return rec->getAddress();
    }
    return Contact();
}


NameServer::NameRecord *NameServer::getNameRecord(const std::string& name,
                                                  bool create) {
    std::map<std::string, NameRecord>::iterator entry = nameMap.find(name);
    if (entry == nameMap.end()) {
        if (!create) {
            return nullptr;
        }
        nameMap[name] = NameRecord();
        entry = nameMap.find(name);
    }
    yAssert(entry != nameMap.end());
    return &(entry->second);
}


NameServer::HostRecord *NameServer::getHostRecord(const std::string& name,
                                                  bool create) {
    std::map<std::string, HostRecord>::iterator entry = hostMap.find(name);
    if (entry == hostMap.end()) {
        if (!create) {
            return nullptr;
        }
        hostMap[name] = HostRecord();
        entry = hostMap.find(name);
        entry->second.setBase(basePort);
    }
    yAssert(entry != hostMap.end());
    return &(entry->second);
}









////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// Remote interface
//
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////



void NameServer::setup() {

    basePort = NetworkBase::getDefaultPortRange()+2;

    dispatcher.add("register", &NameServer::cmdRegister);
    dispatcher.add("unregister", &NameServer::cmdUnregister);
    dispatcher.add("query", &NameServer::cmdQuery);
    dispatcher.add("help", &NameServer::cmdHelp);
    dispatcher.add("set", &NameServer::cmdSet);
    dispatcher.add("get", &NameServer::cmdGet);
    dispatcher.add("check", &NameServer::cmdCheck);
    dispatcher.add("match", &NameServer::cmdMatch);
    dispatcher.add("list", &NameServer::cmdList);
    dispatcher.add("route", &NameServer::cmdRoute);
    dispatcher.add("gc", &NameServer::cmdGarbageCollect);
    dispatcher.add("bot", &NameServer::cmdBot);
    dispatcher.add("announce", &NameServer::cmdAnnounce);

    ndispatcher.add("list", &NameServer::ncmdList);
    ndispatcher.add("query", &NameServer::ncmdQuery);
    ndispatcher.add("version", &NameServer::ncmdVersion);
    ndispatcher.add("set", &NameServer::ncmdSet);
    ndispatcher.add("get", &NameServer::ncmdGet);
}

std::string NameServer::cmdRegister(int argc, char *argv[]) {

    std::string remote = argv[0];
    argc--;
    argv++;

    if (argc<1) {
        return "need at least one argument";
    }
    std::string portName = STR(argv[0]);

    std::string machine = "...";
    std::string carrier = "...";
    int port = 0;
    if (argc>=2) {
        carrier = argv[1];
    }
    if (argc>=3) {
        machine = argv[2];
    }
    if (argc>=4) {
        if (std::string("...") == argv[3]) {
            port = 0;
        } else {
            port = NetType::toInt(argv[3]);
        }
    }

    Contact address = registerName(portName, Contact(portName, carrier, machine, port), remote);

    //YARP_DEBUG(Logger::get(),
    //std::string("name server register address -- ") +
    //address.toString());

    return terminate(textify(address));
}


std::string NameServer::cmdQuery(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<1) {
        return "need at least one argument";
    }
    std::string portName = STR(argv[0]);
    Contact address = queryName(portName);
    return terminate(textify(address));
}

std::string NameServer::cmdUnregister(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<1) {
        return "need at least one argument";
    }
    std::string portName = STR(argv[0]);
    Contact address = unregisterName(portName);
    return terminate(textify(address));
}


std::string NameServer::cmdAnnounce(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    return terminate("ok\n");
}

std::string NameServer::cmdRoute(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<2) {
        return terminate("need at least two arguments: the source port and the target port\n(followed by an optional list of carriers in decreasing order of desirability)");
    }
    std::string src = STR(argv[0]);
    std::string dest = STR(argv[1]);

    argc-=2;
    argv+=2;

    const char *altArgv[] = {
        "local", "shmem", "mcast", "udp", "tcp", "text"
    };
    int altArgc = 6;

    if (argc==0) {
        argc = altArgc;
        argv = (char**)altArgv;
    }


    NameRecord& srcRec = getNameRecord(src);
    NameRecord& destRec = getNameRecord(dest);
    std::string pref = "";

    for (int i=0; i<argc; i++) {
        std::string carrier = argv[i];
        if (srcRec.checkProp("offers", carrier) &&
            destRec.checkProp("accepts", carrier)) {
            bool ok = true;
            if (carrier=="local"||carrier=="shmem") {
                if (srcRec.getProp("ips") == destRec.getProp("ips")) {
                    if (carrier=="local") {
                        if (srcRec.getProp("process") != destRec.getProp("process")) {
                            ok = false;
                        }
                    }
                } else {
                    ok = false;
                }
            }
            if (ok) {
                pref = carrier;
                break;
            }
        }
    }
    if (pref!="") {
        pref = pref + ":/" + dest;
    } else {
        pref = dest;
    }

    std::string result = "port ";
    result += src + " route " + dest + " = " + pref + "\n";
    return terminate(result);
}


std::string NameServer::cmdHelp(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    std::string result = "Here are some ways to use the name server:\n";
    //ACE_Vector<std::string> names = dispatcher.getNames();
    //for (unsigned i=0; i<names.size(); i++) {
    //const std::string& name = names[i];
    //result += std::string("   ") + name + " ...\n";
    //}
    result += std::string("+ help\n");
    result += std::string("+ list\n");
    result += std::string("+ register $portname\n");
    result += std::string("+ register $portname $carrier $ipAddress $portNumber\n");
    result += std::string("  (if you want a field set automatically, write '...')\n");
    result += std::string("+ unregister $portname\n");
    result += std::string("+ query $portname\n");
    result += std::string("+ set $portname $property $value\n");
    result += std::string("+ get $portname $property\n");
    result += std::string("+ check $portname $property\n");
    result += std::string("+ match $portname $property $prefix\n");
    result += std::string("+ route $port1 $port2\n");
    result += std::string("+ gc\n");
    return terminate(result);
}


std::string NameServer::cmdSet(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<2) {
        return "need at least two arguments: the port name, and a key";
    }
    std::string target = STR(argv[0]);
    std::string key = argv[1];
    NameRecord& nameRecord = getNameRecord(target);
    nameRecord.clearProp(key);
    for (int i=2; i<argc; i++) {
        nameRecord.addProp(key, argv[i]);
    }
    return terminate(std::string("port ") + target + " property " + key + " = " +
                     nameRecord.getProp(key) + "\n");
}

std::string NameServer::cmdGet(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<2) {
        return "need exactly two arguments: the port name, and a key";
    }
    std::string target = STR(argv[0]);
    std::string key = argv[1];
    NameRecord& nameRecord = getNameRecord(target);
    return terminate(std::string("port ") + target + " property " + key + " = " +
                     nameRecord.getProp(key) + "\n");
}

std::string NameServer::cmdMatch(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<3) {
        return "need exactly three arguments: the port name, a key, and a prefix";
    }
    std::string target = STR(argv[0]);
    std::string key = argv[1];
    std::string prefix = argv[2];
    NameRecord& nameRecord = getNameRecord(target);
    return terminate(std::string("port ") + target + " property " + key + " = " +
                     nameRecord.matchProp(key, prefix) + "\n");
}

std::string NameServer::cmdCheck(int argc, char *argv[]) {
    // ignore source
    argc--;
    argv++;

    if (argc<2) {
        return "need at least two arguments: the port name, and a key";
    }
    std::string response = "";
    std::string target = STR(argv[0]);
    std::string key = argv[1];
    NameRecord& nameRecord = getNameRecord(target);
    for (int i=2; i<argc; i++) {
        std::string val = "false";
        if (nameRecord.checkProp(key, argv[i])) {
            val = "true";
        }
        if (i>2) {
            response += "\n";
        }
        response += "port ";
        response += target + " property " +
            key + " value " + argv[i] + " present " + val;
    }
    response += "\n";
    return terminate(response);
}


std::string NameServer::cmdList(int argc, char *argv[]) {
    YARP_UNUSED(argc);
    YARP_UNUSED(argv);
    std::string response = "";

    std::multiset<std::string> lines;
    for (auto it = nameMap.begin(); it!=nameMap.end(); ++it) {
        NameRecord& rec = it->second;
        lines.insert(textify(rec.getAddress()));
    }

    // return result in alphabetical order
    for (const auto& line : lines) {
        response += line;
    }

    return terminate(response);
}


std::string NameServer::cmdBot(int argc, char *argv[]) {
    std::string txt = "";
    argc--;
    argv++;
    if (argc>=1) {
        std::string key = argv[0];
        argc--;
        argv++;
        Bottle result = ndispatcher.dispatch(this, key.c_str(), argc, argv);
        txt = result.toString();
    }
    return txt;
}


Bottle NameServer::ncmdList(int argc, char *argv[]) {
    Bottle response;

    std::string prefix = "";

    if (argc==1) {
        prefix = STR(argv[0]);
    }

    response.addString("ports");
    for (auto it = nameMap.begin(); it!=nameMap.end(); ++it) {
        NameRecord& rec = it->second;
        std::string iname = rec.getAddress().getRegName();
        if (iname.find(prefix)==0) {
            if (iname==prefix || iname[prefix.length()]=='/' ||
                prefix[prefix.length()-1]=='/') {
                if (rec.getAddress().isValid()) {
                    response.addList() = botify(rec.getAddress());
                }
            }
        }
    }

    return response;
}


yarp::os::Bottle NameServer::ncmdQuery(int argc, char *argv[]) {
    Bottle response;
    if (argc==1) {
        std::string portName = STR(argv[0]);
        Contact address = queryName(portName);
        response = botify(address);
    }
    return response;
}


yarp::os::Bottle NameServer::ncmdVersion(int argc, char *argv[]) {
    YARP_UNUSED(argc);
    YARP_UNUSED(argv);
    Bottle response;
    response.addString("version");
    response.addString(YARP_VERSION);
    return response;
}


yarp::os::Bottle NameServer::ncmdSet(int argc, char *argv[]) {

    Bottle response;
    if (argc >= 2) {
        std::string target = STR(argv[0]);
        std::string key = STR(argv[1]);
        NameRecord& nameRecord = getNameRecord(target);
        nameRecord.clearProp(key);
        for (int i=2; i<argc; i++) {
            nameRecord.addProp(key, argv[i]);
        }
        response.addString("ok");
    }
    return response;
}

yarp::os::Bottle NameServer::ncmdGet(int argc, char *argv[]) {
    Bottle response;
    if (argc==2) {
        std::string target = STR(argv[0]);
        std::string key = argv[1];
        NameRecord& nameRecord = getNameRecord(target);
        return Bottle(nameRecord.getProp(key));
    }
    return response;
}



std::string NameServer::cmdGarbageCollect(int argc, char *argv[]) {
    YARP_UNUSED(argc);
    YARP_UNUSED(argv);
    std::string response = "";

    response = "No cleaning done.\n";

    return terminate(response);
}


std::string NameServer::textify(const Contact& address) {
    std::string result = "";
    if (address.isValid()) {
        if (address.getPort()>=0) {
            result = "registration name ";
            result = result + address.getRegName() +
                " ip " + address.getHost() + " port " +
                NetType::toString(address.getPort()) + " type " +
                address.getCarrier() + "\n";
        } else {
            result = "registration name ";
            result = result + address.getRegName() +
                " ip " + "none" + " port " +
                "none" + " type " +
                address.getCarrier() + "\n";
        }
    }
    return result;
}


Bottle NameServer::botify(const Contact& address) {
    Bottle result;
    if (address.isValid()) {
        Bottle bname;
        bname.addString("name");
        bname.addString(address.getRegName().c_str());
        Bottle bip;
        bip.addString("ip");
        bip.addString(address.getHost().c_str());
        Bottle bnum;
        bnum.addString("port_number");
        bnum.addInt32(address.getPort());
        Bottle bcarrier;
        bcarrier.addString("carrier");
        bcarrier.addString(address.getCarrier().c_str());

        result.addString("port");
        result.addList() = bname;
        result.addList() = bip;
        result.addList() = bnum;
        result.addList() = bcarrier;
    } else {
        Bottle bstate;
        bstate.addString("error");
        bstate.addInt32(-2);
        bstate.addString("port not known");
        result.addString("port");
        result.addList() = bstate;
    }
    return result;
}


static std::string ns_terminate(const std::string& str) {
    return str + "*** end of message";
}

std::string NameServer::terminate(const std::string& str) {
    return ns_terminate(str);
}


std::string NameServer::apply(const std::string& txt, const Contact& remote) {
    std::string result = "no command given";
    mutex.lock();

    SplitString ss(txt.c_str());
    if (ss.size()>=2) {
        std::string key = ss.get(1);
        //YARP_DEBUG(Logger::get(), std::string("dispatching to ") + key);
        ss.set(1, remote.getHost().c_str());
        result = dispatcher.dispatch(this, key.c_str(), ss.size()-1,
                                     (char **)(ss.get()+1));
        if (result == "") {
            Bottle b = ndispatcher.dispatch(this, key.c_str(), ss.size()-1,
                                            (char **)(ss.get()+1));
            result = b.toString();
            if (result!="") {
                result = result + "\n";
                result = terminate(result);
            }
        }
        //YARP_DEBUG(Logger::get(), std::string("name server request -- ") + txt);
        //YARP_DEBUG(Logger::get(), std::string("name server result  -- ") + result);
    }
    mutex.unlock();
    return result;
}


bool NameServer::apply(const Bottle& cmd, Bottle& result,
                       const Contact& remote) {
    Bottle rcmd;
    rcmd.addString("ignored_legacy");
    rcmd.append(cmd);
    std::string in = rcmd.toString();
    std::string out = apply(in, remote);
    result.fromString(out);
    return true;
}



#ifndef DOXYGEN_SHOULD_SKIP_THIS

class MainNameServerWorker : public PortReader {
private:
    NameServer *server;
public:
    MainNameServerWorker(NameServer *server) {
        this->server = server;
    }

    virtual bool read(ConnectionReader& reader) override {
        YTRACE("NameServer::read start");
        std::string ref = "NAME_SERVER ";
        bool ok = true;
        std::string msg = "?";
        bool haveMessage = false;
        if (ok) {
            if (reader.isTextMode()) {
                msg = reader.expectText();
            } else {
                // migrate to binary mode support, eventually optimize
                Bottle b;
                b.read(reader);
                msg = b.toString();
            }
            haveMessage = (msg!="");
            msg = ref + msg;
        }
        if (reader.isActive()&&haveMessage) {
            YARP_DEBUG(Logger::get(), std::string("name server got message ") + msg);
            size_t index = msg.find("NAME_SERVER");
            if (index==0) {
                Contact remote = reader.getRemoteContact();
                YARP_DEBUG(Logger::get(),
                           std::string("name server receiving from ") +
                           remote.toURI());
                YARP_DEBUG(Logger::get(),
                           std::string("name server request is ") + msg);
                std::string result = server->apply(msg, remote);
                ConnectionWriter *os = reader.getWriter();
                if (os!=nullptr) {
                    if (result=="") {
                        result = ns_terminate(std::string("unknown command ") +
                                              msg + "\n");
                    }
                    // This change is just to make Microsoft Telnet happy
                    std::string tmp;
                    for (unsigned int i=0; i<result.length(); i++) {
                        if (result[i]=='\n') {
                            tmp += '\r';
                        }
                        tmp += result[i];
                    }
                    tmp += '\r';
                    os->appendString(tmp.c_str(), '\n');

                    YARP_DEBUG(Logger::get(),
                               std::string("name server reply is ") + result);
                    std::string resultSparse = result;
                    size_t end = resultSparse.find("\n*** end of message");
                    if (end!=std::string::npos) {
                        resultSparse[end] = '\0';
                    }
                    YARP_INFO(Logger::get(), resultSparse);
                }
            } else {
                YARP_INFO(Logger::get(),
                          std::string("Name server ignoring unknown command: ")+msg);
            }
        }
        YTRACE("NameServer::read stop");
        return true;
    }
};


class MainNameServer : public NameServer, public PortReaderCreator {
private:
    Port *port;
public:
    MainNameServer(int basePort, Port *port = nullptr) : port(port) {
        setBasePort(basePort);
    }

    void setPort(Port& port) {
        this->port = &port;
    }

    virtual void onEvent(Bottle& event) override {
        if (port!=nullptr) {
            port->write(event);
        }
    }

    virtual PortReader *create() const override {
        return new MainNameServerWorker(const_cast<MainNameServer*>(this));
    }
};


#endif /* DOXYGEN_SHOULD_SKIP_THIS */
