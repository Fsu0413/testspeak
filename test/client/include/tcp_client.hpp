#ifndef __TCP_CLIENT_HPP__
#define __TCP_CLIENT_HPP__

#define __DEBUG__ 1

//#define __GNU_LINUX__ 1
//#define __MS_WIN32__ 1

#include <string>
#include <sys/types.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
//#pragma comment(lib, "wsock32.lib")
//#pragma comment(lib, "ws2_32.lib")
//typedef unsigned short int uint16_t;
//typedef unsigned long int uint32_t;
//typedef int ssize_t;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
const int INVALID_SOCKET = -1;
#endif //__GNU_LINUX__

class TCPClient
{
private:
#ifdef _WIN32
    WSADATA wsaData;
#endif // __MS_WIN32__

    SOCKET _sock;
    struct sockaddr_in _dst_addr_in;

    bool _init_flag;

    std::string _err_mesg;
    int _err_no;

    int create_socket(uint32_t, const char *);

public:
    TCPClient();
    ~TCPClient();

    int connect(uint32_t, std::string);
    int connect(uint32_t, const char *);
    int disconnect();
    int send(const std::string, int flags = 0);
    int send(const char *, int flags = 0);
    int send(const char *, int, int flags = 0);
    int recive(char *, int, int flags = 0);

    int get_socket();

    // for debug user only
#ifdef __DEBUG__
    int get_error_no();
    std::string get_error_mesg();
#endif // __DEBUG__
};

int hostname2ipaddr(const char *, char *);
std::string hostname2ipaddr(const char *);
std::string hostname2ipaddr(const std::string &);

#endif
