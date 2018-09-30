#include "http_client.hpp"
#include "tcp_client.hpp"
#include "json/reader.h"
#include "json/writer.h"

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

struct tlData
{
    tlData(const string &key, const string &userId)
        : key(key)
        , userId(userId)
    {
    }

    string key;
    string userId;
};

vector<tlData> TLData;

string talk(const string &toSend)
{
    Json::Value v(Json::objectValue);
    v["key"] = TLData.at(0).key;
    v["userid"] = TLData.at(0).userId;
    v["info"] = toSend;

    Json::FastWriter writer;
    string sendData = writer.write(v);
    string url("http://www.tuling123.com/openapi/api");
    //    string url("http://www.tuling123.com/openapi/api?");
    //    url.append("key=");
    //    url.append(TLData.at(1).key);
    //    url.append("&userid=");
    //    url.append(TLData.at(1).userId);
    //    url.append("&info=");
    //    url.append(toSend);

    HTTPHeader header;
    header.insert("Content-Type", "text/json");

    std::string body;
    for (int i = 0; i < 15; ++i) {
        HTTPClient client;
        client.post(&url, &sendData, &header);
        if (client.status_code() != 200) {
            cout << "client error, code = " << client.status_code() << endl;
        } else {
            body = *client.body();
            break;
        }
    }

    if (body.empty()) {
        cout << "client error 15 times" << endl;
    } else {
        Json::Reader reader;
        Json::Value value;
        reader.parse(body, value);

        if (value.isMember("code")) {
            int x = value["code"].asInt();

            switch (x) {
            case 100000: {
                string st = value["text"].asString();
                regex re("\\s");
                return regex_replace(st, re, "");
            }
            default:
                return "Hello";
            }
        } else {
            cout << "parse error?" << endl;
        }
    }

    return "Hello";
}

#ifndef _WIN32
static inline void Sleep(uint32_t t)
{
    usleep(t * 1000);
}
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <random>

void randomSleep(const string &str)
{
    int i = 1500;
    int a = 7500;

    int n = 0;
    for (int x = 0; x < str.size(); ++x) {
        n += (std::random_device()() % 60 + 15);
    }

    Sleep(std::random_device()() % (a - i) + i + n);
}

int main(int argc, char *argv[])
{
    TLData.push_back(tlData(string("095669531b59423db7f615dfa84771f5"), string("v1")));

    for (;;) {
        TCPClient *client = new TCPClient;
        if (client->connect(40000, "192.168.1.70") == 0) {
            for (;;) {
                ostringstream oss1;
                char x[2] = {0, 0};
                bool f = false;
                while (x[0] != '\n') {
                    int ret = client->recive(x, 1, 0);
                    if (ret <= 0) {
                        f = true;
                        break;
                    }
                    oss1 << string(x);
                }
                if (f)
                    break;

                cout << "Received: " << oss1.str();
                ostringstream oss2;
                oss2 << talk(oss1.str()) << endl;
                string str = oss2.str();
                cout << "Sending: " << str;
                //randomSleep(str);
                if (client->send(str) <= 0)
                    break;
            }
        }
        client->disconnect();
        delete client;
    }

    return 0;
}
