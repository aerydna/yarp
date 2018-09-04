#include <yarp/server/yarpserver.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/ConnectionReader.h>
#include <yarp/os/ConnectionWriter.h>
#include <yarp/os/Vocab.h>
#include <string>
#include <yarp/os/LogStream.h>
using namespace yarp::server;
using namespace yarp::os;
using namespace std;

constexpr int startingp = 20000;



//*******************************************//
//***************  UTILITIES  ************** //
//*******************************************//

void bottleToStringv(vector<string>& v, const Bottle& b)
{
    v.clear();
    for (int i = 0; i < b.size(); i++)
    {
        v.push_back(b.get(i).asString());
    }
}

Bottle contactToBottle(const Contact& c)
{
    Bottle r;
    r.addString("registration");
    r.addString("name");
    r.addString(c.getName());
    r.addString("ip");
    r.addString(c.getHost());
    r.addString("port");
    r.addInt32(c.getPort());
    r.addString("type");
    r.addString(c.getCarrier());
    return r;
}



//*******************************************//
//***************  PROTOCOL  ****************//
//*******************************************//

class Request
{
public:
    virtual bool fromBottle(const Bottle& cmd) = 0;
};

class Reply
{
public:
    virtual Bottle toBottle() = 0;
};

class AnnounceRequest : Request
{
public:
    string name;
    bool active{true};

    virtual bool fromBottle(const Bottle& cmd) override
    {
        if (!cmd.size() || !cmd.get(0).isString()) return false;
        name   = cmd.get(0).asString();


        if(cmd.size() > 1 && cmd.get(1).isInt32()) active = cmd.get(1).asInt32() > 0;
        return true;
    }
};

class RegisterRequest : Request
{
public:
    string name;
    virtual bool fromBottle(const Bottle& cmd) override
    {
        if (!cmd.size() || !cmd.get(0).isString()) return false;
        name = cmd.get(0).asString();
        return true;
    }
};

class SetGetMsg
{
public:
    SetGetMsg() {}
    SetGetMsg(const string& cName, const string& pName, vector<string>& values) : contactName(cName), parameterName(pName), values(values) {}
    string         contactName;
    string         parameterName;
    vector<string> values;
};

class SetCmdRequest : public Request,
                      public SetGetMsg
{
public:
    SetCmdRequest() {}
    SetCmdRequest(const string& cName, const string& pName, vector<string>& values) : SetGetMsg(cName, pName, values) {};
    virtual bool fromBottle(const Bottle& cmd) override
    {
        if (!cmd.size() || !cmd.get(0).isString()) return false;
        contactName = cmd.get(0).asString();
        parameterName = cmd.get(1).asString();
        bottleToStringv(values, cmd.tail().tail());
        return true;
    }
};

class GetCmdReply : public Reply,
                    public SetGetMsg
{
public:
    GetCmdReply(const string& cName, const string& pName, vector<string>& values) : SetGetMsg(cName, pName, values) {};
    virtual Bottle toBottle() override
    {
        Bottle r;
        r.addString("port");
        r.addString(contactName);
        r.addString("property");
        r.addString(parameterName);
        r.addString("=");
        for (auto v : values)
        {
            r.addString(v);
        }

        return r;
    }
};



//*******************************************//
//**********  PORT DATA STRUCTURES **********//
//*******************************************//

struct Param
{
    std::string         name;
    vector<std::string> value;
    std::vector<Param> subparam;
};

struct ParamContact : public yarp::os::Contact
{
    ParamContact() : Contact(){}
    ParamContact(Contact c) : Contact(c){}

    std::map<string, Param> params;
    bool live{false};
};

typedef std::map<std::string, ParamContact> PortMap;

struct PortStorage
{
    PortMap storage;
    const string temporaryPortMsg{ "..." };
    int currentPort;
    PortStorage(int s_port) : currentPort(s_port) {}
    
    int getNextPortNumber()
    {
        currentPort++;
        return currentPort;
    }
    
    bool setParam(const string& contactName, const string& paramName, const vector<string>& value)
    {
        if (storage.find(contactName) == storage.end()) return false;
        storage[contactName].params[paramName] = { paramName, value };
    }

    std::vector<string> getParam(const string contactName, const string paramName) const
    {
        if (storage.find(contactName) == storage.end() || storage.at(contactName).params.find(paramName) != storage.at(contactName).params.end()) return std::vector<string>();
        return storage.at(contactName).params.at(paramName).value;
    }

    Contact registerPort(const string& name, Contact c, bool newsocket = false)
    {
        newsocket = name == temporaryPortMsg ? true : newsocket;

        if (newsocket)
        {
            c.setPort(getNextPortNumber());
        }

        if (name == temporaryPortMsg) c.setName("/temporary_port/" + std::to_string(c.getPort()));
        else c.setName(name);

        storage[c.getName()] = c;
        return c;
    }

    Contact query(string name)
    {
        if (storage.find(name) == storage.end()) return Contact();
        return storage[name];
    }

    int unregisterPort(const string& name)
    {
        return storage.erase(name);
    }

    bool announcePort(const string name, bool active)
    {
        if (storage.find(name) == storage.end()) return false;
        storage.at(name).live = active;
        return true;
    }
};

class BottleWithSender : public PortReader,
                         public PortWriter
{
public:
    Bottle msg;
    Contact sender;
    virtual bool read(ConnectionReader& reader) override
    {
        msg.clear();
        sender = reader.getRemoteContact();
        return msg.read(reader);
    }

    virtual bool write(ConnectionWriter& writer) const override
    {
        //writer.appendString("*** end of message");
        return msg.write(writer);
    }
};



//*******************************************//
//*********  NAMESERVER IMPLEMENTATION  *****//
//*******************************************//

void yarp::server::NameServer::start()
{
    yarp::os::Contact c;
    BottleWithSender  cmd;
    vector<string>    cmds;
    PortStorage       ports(startingp);
    Contact           requester;

    c.setName("/yarp");
    c.setPort(startingp);
    c.setHost("192.168.10.30");
    ports.registerPort("/yarp", Contact("192.168.10.30", startingp));
    cmdReceiver.open(c, false);
    yInfo() << "I man! yarpserver here.. whatcha doing? how can i help ya?";
    while (!mustStop)
    {
        cmdReceiver.read(cmd, true);
        bottleToStringv(cmds, cmd.msg);
        if (!cmd.msg.size() || !cmd.msg.get(0).isString())
        {
            Bottle b;
            b.addString("request not understood");
            cmdReceiver.reply(b);
            continue;
        }

        string command = cmd.msg.get(0).asString();

        if (command == "register")
        {
            RegisterRequest r;
            if (!r.fromBottle(cmd.msg.tail()))
            {
                cmdReceiver.reply(contactToBottle(Contact()));
                continue;
            }
            yInfo() << "registering" << r.name;
            cmdReceiver.reply(contactToBottle(ports.registerPort(r.name, cmd.sender, true)));
        }

        if (command == "set")
        {
            SetCmdRequest r;
            if (!r.fromBottle(cmd.msg.tail()))
            {
                continue;
            }
            yInfo() << "setting" << r.parameterName << "parameter on contact" << r.contactName << "with value/s:" << r.values;
            ports.setParam(r.contactName, r.parameterName, r.values);
            auto v = ports.getParam(r.contactName, r.parameterName);

            cmdReceiver.reply(GetCmdReply(r.contactName, r.parameterName, v).toBottle());
        }

        if (command == "unregister")
        {
            Bottle b;
            if (!cmd.msg.get(1).isString())
            {
                b.addString("unregister name parameter not found");
                cmdReceiver.reply(b);
                continue;
            }
            yInfo() << "deregistering" << cmd.msg.get(1).asString();
            string portname = cmd.msg.get(1).asString();
            ports.unregisterPort(portname);
            cmdReceiver.reply(b);
            //cmdReceiver.reply(contactToBottle(ports.query(portname)));
        }

        if (command == "query")
        {
            if (!cmd.msg.get(1).isString())
            {
                Bottle b;
                b.addString("query name parameter not found");
                cmdReceiver.reply(b);
                continue;
            }
            yInfo() << "query" << cmd.msg.get(1).asString();
            cmdReceiver.reply(contactToBottle(ports.query(cmd.msg.get(1).asString())));
        }

        if (command == "announce")
        {
            Bottle r;
            AnnounceRequest req;
            if(req.fromBottle(cmd.msg.tail())) ports.announcePort(req.name, req.active);

            string ac = req.active ? "active" : "inactive";
            yInfo() << "announcing" << req.name << "to be" << ac;
            //in the old name server, fail cases were ignored
            r.addVocab(yarp::os::createVocab('o', 'k'));
            cmdReceiver.reply(r);
        }
    }
}