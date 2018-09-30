#ifndef SERVER_H
#define SERVER_H

namespace asio {
class io_context;
}

class Server
{
public:
    explicit Server(asio::io_context &context);
    ~Server();

private:
    Server(const Server &) = delete;
    Server *operator=(const Server &) = delete;
};

#endif
