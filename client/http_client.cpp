#include <http_client.hpp>
#include <http_client_utils.hpp>
#include <iostream>
#include <sstream>
#include <string>

HTTPClient::HTTPClient() {
  recv_status = new std::string();
  recv_header = new HTTPHeader();
  recv_body = new std::string();
}

HTTPClient::~HTTPClient() {
  delete recv_body;
  delete recv_header;
  delete recv_status;
}

int HTTPClient::splitURL(const std::string *url, std::string *ptcl,
                         std::string *addr, std::string *file, int *port) {
  std::string *_url = new std::string(*url);

  std::string::size_type protocol_pos_end = _url->find_first_of("://");
  ptcl->assign(_url->substr(0, protocol_pos_end));
  if (!((ptcl->compare("http") == 0) || (ptcl->compare("https") == 0)))
    return -1;

  _url->erase(0, protocol_pos_end + 3);

  std::string::size_type addr_pos_end = _url->find_first_of("/", 0);
  addr->assign(_url->substr(0, addr_pos_end));

  file->assign(_url->erase(0, addr_pos_end));
  if (file->length() == 0)
    file->assign("/");

  std::string::size_type post_pos_begin = addr->find_last_of(":");
  if (post_pos_begin == std::string::npos) {
    *port =
        (ptcl->compare("https") == 0) ? DEFAULT_HTTPS_PORT : DEFAULT_HTTP_PORT;
  } else {
    *port = atoi(addr->substr(post_pos_begin + 1).c_str());
    addr->erase(post_pos_begin);
  }

  delete _url;

  return 0;
}

uint32_t HTTPClient::receive_by_line(TCPClient *client, char *buf,
                                     uint32_t buf_size) {
  bool isHead = true;
  uint32_t received = 0;
  memset(buf, 0, buf_size);
  bool chunk = false;
  int contentLength = 0;
  while (received < buf_size) {
    char temp;

    uint32_t recv_size = client->recive(&temp, 1);
    if (recv_size == 0)
      break;
    else if (recv_size == -1)
      break;

    buf[received++] = temp;

    if (temp == '\n') {
      // split head and body
      if (isHead && ((buf[received - 4] == buf[received - 2]) &&
                     buf[received - 4 == '\r'] && buf[received - 3] == '\n')) {
        isHead = false;

        std::string _headerStr(buf, received - 2);
        std::string _headerStrLf =
            NewLineCode::convert(_headerStr, NewLineCode::LF);

        HTTPHeader _header;
        _header.set(_headerStrLf);

        if (_header.get(std::string("Transfer-Encoding")) ==
            std::string("chunked")) {
          chunk = true;
        } else {
          std::string length = _header.get(std::string("Content-Length"));
          if (length.length() != 0) {
            std::istringstream iss(length);
            iss >> contentLength;
            uint32_t oversize = 0;
            if (contentLength + received > buf_size) {
              oversize = (contentLength + received) - buf_size;
              contentLength -= oversize;
            }

            recv_size = client->recive(buf + received, contentLength);

            received += recv_size;
            break;
          }
        }
      } else if (!isHead && chunk) {
        if (memcmp((const void *)("\r\n0\r\n\r\n"), buf + received - 7, 7) == 0)
          break;
      }
    }
  }

  return received;
}

int HTTPClient::send_and_recive(std::string *addr, int port, std::string *mesg,
                                std::string *recv_str) {
  const uint32_t recv_buf_size = 2000;
  char *recv_buf = new char[recv_buf_size + 1];
  memset(recv_buf, 0, recv_buf_size + 1);

  TCPClient *tcp_client = new TCPClient();
  tcp_client->connect(port, hostname2ipaddr(addr->c_str()).c_str());
  tcp_client->send(*mesg);
  //    while (true) {
  //      uint32_t recv_size = tcp_client->recive(recv_buf, recv_buf_size);
  //      if (recv_size == -1)
  //        return 1;

  //      if (recv_str->max_size() <= recv_buf_size + recv_str->length())
  //        break;
  //      recv_buf[recv_size] = '\0';
  //      recv_str->append(recv_buf);

  //      if (recv_size == 0)
  //        break;
  //    }

  receive_by_line(tcp_client, recv_buf, recv_buf_size);
  recv_str->append(recv_buf);

  tcp_client->disconnect();
  delete tcp_client;

  delete recv_buf;

  return 0;
}

void assign_chunked_body(std::string *recv_body, const std::string *body) {
  std::istringstream iss(*body);
  std::ostringstream oss;
  std::string::size_type p;
  iss >> std::hex >> p;

  while (p > 0) {
    while (iss.peek() == '\n')
      iss.get();

    char *buf = new char[p + 5];
    iss.getline(buf, p + 1);
    oss << buf;
    delete[] buf;

    if (iss.eof())
      break;

    iss >> std::hex >> p;
  }

  recv_body->assign(oss.str());
}

int HTTPClient::split_header_body(std::string *_src) {
  std::string src = NewLineCode::convert(*_src, NewLineCode::LF);
  std::string::size_type p = src.find("\n");

  recv_status->assign(src.substr(0, p));

  std::string::size_type q = src.find("\n\n", p + 1);

  std::string *_header = new std::string();
  _header->assign(src.substr(p + 1, q - p - 1));
  recv_header->set(*_header);
  delete _header;

  if (recv_header->get(std::string("Transfer-Encoding")) ==
      std::string("chunked")) {
    std::string *_body = new std::string;
    _body->assign(src.substr(q));
    assign_chunked_body(recv_body, _body);
    delete _body;
  } else {
    recv_body->assign(src.substr(q + 2));
  }

  return 0;
}

int HTTPClient::get(const std::string *url, HTTPHeader *header) {
  std::string *ptcl = new std::string();
  std::string *addr = new std::string();
  std::string *file = new std::string();
  int port = 0;

  if (splitURL(url, ptcl, addr, file, &port) < 0)
    return -1;

  header->insert(std::string("Host"), *addr);
  header->insert(std::string("User-Agent"), UserAgent);
  std::string *mesg = new std::string("GET " + *file + " HTTP/1.1\r\n" +
                                      header->to_string() + "\r\n");

  std::string *recv_str = new std::string();
  send_and_recive(addr, port, mesg, recv_str);
  split_header_body(recv_str);
  delete recv_str;

  delete file;
  delete addr;
  delete ptcl;

  return 0;
}

int HTTPClient::post(const std::string *url, const std::string *post,
                     HTTPHeader *header) {
  std::string *ptcl = new std::string();
  std::string *addr = new std::string();
  std::string *file = new std::string();
  int port = 0;

  if (splitURL(url, ptcl, addr, file, &port) < 0)
    return -1;

  header->insert(std::string("Host"), *addr);
  header->insert(std::string("User-Agent"), UserAgent);
  header->insert(std::string("Content-Length"), int2str(post->length()));
  std::string *mesg = new std::string("POST " + *file + " HTTP/1.1\r\n" +
                                      header->to_string() + "\r\n" + *post);

  std::string *recv_str = new std::string();
  send_and_recive(addr, port, mesg, recv_str);
  split_header_body(recv_str);
  delete recv_str;

  delete file;
  delete addr;
  delete ptcl;

  return 0;
}

int HTTPClient::status_code() {
  std::string::size_type p = recv_status->find(' ') + 1;
  std::string::size_type q = recv_status->find(' ', p + 1);
  return str2int(recv_status->substr(p, q - p));
}
std::string *HTTPClient::status() { return recv_status; }
HTTPHeader *HTTPClient::header() { return recv_header; }
std::string *HTTPClient::body() { return recv_body; }
