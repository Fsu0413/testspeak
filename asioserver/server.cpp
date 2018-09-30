#include "server.h"
#include "asio.hpp"
#include "json/json.h"
#include <iostream>
#include <map>

enum ServerProtocol
{
    SP_Zero,

    SP_SignIn,
    SP_Query,
    SP_Speak,

    SP_Max,
};

enum ClientProtocol
{
    CP_Zero,

    CP_SignedIn,
    CP_SignedOut,
    CP_QueryResult,
    CP_Spoken,

    CP_Max,
};

class TcpSocketWrapper
{
public:
    explicit TcpSocketWrapper(asio::ip::tcp::socket &&socketRvalueRef)
        : socket(std::move(socketRvalueRef))
        , lineFunc(nullptr)
        , failFunc(nullptr)

    {
    }

    asio::ip::tcp::socket *s()
    {
        return &socket;
    }

    const asio::ip::tcp::socket *s() const
    {
        return &socket;
    }

    void registerLineRead(void (*func)(TcpSocketWrapper *self, const std::string &line))
    {
        lineFunc = func;
    }

    void registerFail(void (*func)(TcpSocketWrapper *self))
    {
        failFunc = func;
    }

private:
    void doAsyncReadLine()
    {
        asio::async_read_until(socket, buf, '\n', [this](const asio::error_code &e, std::size_t) {
            if (!e) {
                std::istream st(&buf);
                std::string line;
                std::getline(st, line);
                doAsyncReadLine();
                if (lineFunc != nullptr)
                    (*lineFunc)(this, line);

            } else {
                if (failFunc != nullptr)
                    (*failFunc)(this);
            }
        });
    }

private:
    asio::ip::tcp::socket socket;

    asio::streambuf buf;
    void (*lineFunc)(TcpSocketWrapper *self, const std::string &line);
    void (*failFunc)(TcpSocketWrapper *self);
    TcpSocketWrapper(const TcpSocketWrapper &) = delete;
    TcpSocketWrapper &operator=(const TcpSocketWrapper &) = delete;
};

void writeJsonDocument(TcpSocketWrapper *wrapper, const Json::Value &value);

struct ServerImpl
{
    explicit ServerImpl(asio::io_context &context)
        : acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 40001))
    {
    }

    asio::ip::tcp::acceptor acceptor;

    typedef void (ServerImpl::*ServerFunction)(TcpSocketWrapper *socket, const Json::Value &content);

    static const ServerFunction ServerFunctions[SP_Max];

    std::map<TcpSocketWrapper *, Json::Value> socketTlMap;
    std::map<std::string, TcpSocketWrapper *> nameSocketMap;

    void listen();
    void disconnect(TcpSocketWrapper *wrapper);

    void signInFunc(TcpSocketWrapper *socket, const Json::Value &content)
    {
        socketTlMap[socket] = content;
        auto copy = content;
        copy["protocolValue"] = int(CP_SignedIn);

        for (auto it = nameSocketMap.begin(); it != nameSocketMap.end(); ++it)
            writeJsonDocument(it->second, copy);

        std::string name = content["userName"].asString();
        nameSocketMap[name] = socket;

        auto ep = socket->s()->remote_endpoint();
        std::cout << ep.address().to_string() << "," << ep.port() << "registered as" << name << std::endl;
    }

    void queryFunc(TcpSocketWrapper *socket, const Json::Value &content)
    {
        Json::Value ret(Json::objectValue);
        auto ep = socket->s()->remote_endpoint();
        if (!content.isMember("userName")) {
            std::cout << ep.address().to_string() << "," << ep.port() << "is querying all" << std::endl;
            Json::Value arr(Json::arrayValue);
            Json::Value ob(Json::objectValue);

            for (auto it = socketTlMap.begin(); it != socketTlMap.end(); ++it) {
                arr.append(it->second["userName"]);
                ob[it->second["userName"].asString()] = it->second;
            }

            ret["res"] = arr;
            ret["resv2"] = ob;

        } else {
            std::string name = content["userName"].asString();
            std::cout << ep.address().to_string() << "," << ep.port() << "is querying " << name << std::endl;
            auto f = nameSocketMap.find(name);
            if (f != nameSocketMap.cend())
                ret = socketTlMap[f->second];
        }

        ret["protocolValue"] = int(CP_QueryResult);

        writeJsonDocument(socket, ret);
    }

    void speakFunc(TcpSocketWrapper *socket, const Json::Value &content)
    {
        std::list<TcpSocketWrapper *> tos;
        for (auto it = nameSocketMap.begin(); it != nameSocketMap.cend(); ++it)
            tos.push_back(it->second);

        auto ep = socket->s()->remote_endpoint();
        std::cout << ep.address().to_string() << "," << ep.port() << "is speaking";

        if (content.isMember("to")) {
            std::string name = content["to"].asString();
            std::cout << "to" << name << std::endl;
            if (nameSocketMap.find(name) != nameSocketMap.cend()) {
                tos.clear();
                tos.push_back(socket);
                tos.push_back(nameSocketMap[name]);
            } else {
                return;
            }
        } else
            std::cout << "to all" << std::endl;

        auto copy = content;
        copy["protocolValue"] = int(CP_Spoken);
        copy["time"] = Json::Int64(time(nullptr));

        for (auto it = tos.begin(); it != tos.end(); ++it)
            writeJsonDocument(*it, copy);
    }

    void heartbeatFunc(TcpSocketWrapper *socket, const Json::Value &content)
    {
        (void)content;

        Json::Value v(Json::objectValue);
        v["protocolValue"] = int(CP_Zero);

        writeJsonDocument(socket, v);
    }
};

const ServerImpl::ServerFunction ServerImpl::ServerFunctions[SP_Max] = {&ServerImpl::heartbeatFunc, &ServerImpl::signInFunc, &ServerImpl::queryFunc, &ServerImpl::speakFunc};

ServerImpl *serverImplInstance = nullptr;

void lineRead(TcpSocketWrapper *self, const std::string &line)
{
    Json::Value v;
    std::string e;
    std::istringstream s(line);
    bool result = Json::parseFromStream(Json::CharReaderBuilder(), s, &v, &e);
    if (result) {
        if (v.isObject()) {
            int protocolValue = v["protocalValue"].asInt();
            auto ep = self->s()->remote_endpoint();
            std::cout << ep.address().to_string() << "," << ep.port() << "sent protocol value" << protocolValue << std::endl;

            if (protocolValue >= SP_Zero && protocolValue < SP_Max) {
                auto func = ServerImpl::ServerFunctions[protocolValue];

                if (func != nullptr)
                    (serverImplInstance->*func)(self, v);
            }
        }
    }
}

void fail(TcpSocketWrapper *self)
{
    serverImplInstance->disconnect(self);
}

void writeJsonDocument(TcpSocketWrapper *wrapper, const Json::Value &value)
{
    Json::FastWriter writer;
    std::string s = writer.write(value);
    if (!asio::write(*wrapper->s(), asio::buffer(s.data(), s.length()))) {
        // do nothing
    } else {
        // error
    }
}

Server::Server(asio::io_context &context)
{
    serverImplInstance = new ServerImpl(context);
    serverImplInstance->listen();
}

Server::~Server()
{
    delete serverImplInstance;
}

void ServerImpl::listen()
{
    acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        (void)ec;

        auto wrapper = new TcpSocketWrapper(std::move(socket));
        auto ep = wrapper->s()->remote_endpoint();
        std::cout << ep.address().to_string() << "," << ep.port() << " connected." << std::endl;
        wrapper->registerLineRead(&lineRead);
        wrapper->registerFail(&fail);
    });
}

void ServerImpl::disconnect(TcpSocketWrapper *wrapper)
{
    auto ep = wrapper->s()->remote_endpoint();
    std::cout << ep.address().to_string() << "," << ep.port() << " disconnected." << std::endl;
    std::string name = socketTlMap[wrapper]["userName"].asString();
    socketTlMap.erase(wrapper);
    nameSocketMap.erase(name);
    delete wrapper;

    Json::Value ob(Json::objectValue);

    ob["protocolValue"] = int(CP_SignedOut);
    ob["userName"] = name;

    for (auto it = nameSocketMap.begin(); it != nameSocketMap.end(); ++it)
        writeJsonDocument(it->second, ob);
}
