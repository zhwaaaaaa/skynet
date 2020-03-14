//
// Created by dear on 2020/2/17.
//
#include <iostream>
#include <registry/ZkNamingServer.h>
#include <glog/logging.h>

using namespace std;
using namespace sn;

int main() {
    ZkConfig config(
            "hadoop0:2181,hadoop1:2181,hadoop2:2181",
            5000,
            "",
            "skynet",
            "192.168.106.102:9999"
    );
    ZkNamingServer zkNamingServer(config);
    auto name = "com.zhw.skynet.EchoService";


    zkNamingServer.subscribe(name, [](const string_view &service, const vector<string> &val, void *param) {
        cout << "===========start=======================" << endl;
        for (const auto &s:val) {
            cout << s << endl;
        }
        cout << "===========end======================" << endl;
    }, nullptr);

    zkNamingServer.registerService(name);
    unique_ptr<vector<string>> ptr = zkNamingServer.lookup(name);
    CHECK(!ptr->empty());
    for (const auto &s:*ptr) {
        cout << s << endl;
    }

    zkNamingServer.unregisterService(name);
    ptr = zkNamingServer.lookup(name);
    CHECK(ptr->empty());

    zkNamingServer.unsubscribe(name);
    zkNamingServer.registerService(name);
    return 0;
}