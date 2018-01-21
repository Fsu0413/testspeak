#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#define __DEBUG__ 1

#include <algorithm>
#include <map>
#include <new_line.hpp>
#include <pthread.h>
#include <string>
#include <tcp_client.hpp>

#define DEFAULT_HTTP_PORT 80
#define DEFAULT_HTTPS_PORT 443

typedef std::pair<std::string, std::string> HTTPHeaderLine;

const std::string UserAgent("TwitterTest");

class HTTPHeader {
private:
  const std::string _delimiter = ": ";
  const std::string _cr_lf = "\r\n";
  std::map<std::string, std::string> *_header;
  std::map<std::string, std::string> str_split(const std::string &);

public:
  HTTPHeader();
  ~HTTPHeader();
  int insert(const std::string, const std::string);
  int insert(const HTTPHeaderLine);
  int erase(const std::string);
  int modify(const std::string, const std::string);
  int modify(const HTTPHeaderLine);
  std::string to_string();
  int to_string(std::string *);
  std::string get(const std::string &);
  int get(std::string *, std::string *);
  int set(std::string);
  // void print();
};

class HTTPClient {
private:
  TCPClient *_tcpClient;

  std::string *recv_status;
  HTTPHeader *recv_header;
  std::string *recv_body;

  int splitURL(const std::string *, std::string *, std::string *, std::string *,
               int *);
  int send_and_recive(std::string *, int, std::string *, std::string *);
  uint32_t receive_by_line(TCPClient *, char *, uint32_t);
  int split_header_body(std::string *);

public:
  HTTPClient();
  ~HTTPClient();
  int get(const std::string *, HTTPHeader *);
  int post(const std::string *, const std::string *, HTTPHeader *);
  int status_code();
  std::string *status();
  HTTPHeader *header();
  std::string *body();
};

#endif
