#include "http_client.hpp"
#include "tcp_client.hpp"
#include "json/reader.h"
#include "json/writer.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

struct tlData
{
    tlData(string key, string userId)
        : key(key)
        , userId(userId)
    {
    }

    string key;
    string userId;
};

vector<tlData> TLData;

string talk(string toSend)
{
    Json::Value v(Json::objectValue);
    v["key"] = TLData.at(4).key;
    v["userid"] = TLData.at(4).userId;
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
            case 100000:
                return value["text"].asString();
                break;
            default:
                return "Hello";
            }
        } else {
            cout << "parse error?" << endl;
        }
    }

    return "Hello";
}

extern "C" int main(int argc, char *argv[])
{
    TLData.push_back(tlData(string("884b7214b1774e74963fa30f468126aa"), string("test1")));
    TLData.push_back(tlData(string("15ba950f34fb437dbfc1ab35c7b23949"), string("test2")));
    TLData.push_back(tlData(string("9b9b40047d9c43e8b4789c0f35ae78c1"), string("test3")));
    TLData.push_back(tlData(string("93582ff93fe64a69b5832c2d6519d868"), string("test4")));
    TLData.push_back(tlData(string("f3ebf25d77084a63a0f874c83f2c7418"), string("test5")));

    TCPClient client;
    if (client.connect(40000, "192.168.1.70") == 0) {
        std::string hello = "Hello\n";
        cout << "Sent: " << hello;
        client.send(hello);

        for (;;) {
            ostringstream oss1;
            char x[2] = {0, 0};
            while (x[0] != '\n') {
                int ret = client.recive(x, 1, 0);
                if (ret == -1) {
                    // ??
                }
                oss1 << string(x);
            }
            cout << "Received: " << oss1.str() << endl;
            ostringstream oss2;
            oss2 << talk(oss1.str()) << endl;
            cout << "Sent: " << oss2.str();
            //sleep(3);
            client.send(oss2.str());
        }
    }

    return 0;
}
