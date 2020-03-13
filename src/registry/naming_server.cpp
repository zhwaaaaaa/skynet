//
// Created by dear on 20-1-29.
//

#include "naming_server.h"

namespace sn {


    unique_ptr<vector<string>>
    DemoNamingServer::subscribe(const string_view &serviceName, SubscribeFunc func, void *param) {
        subscribeMap.insert(make_pair(serviceName, FuncWithParam({func, param})));
        func(serviceName, {"127.0.0.1:9999"}, param);
    }

    void DemoNamingServer::unsubscribe(const string_view &serviceName) {
        auto iterator = subscribeMap.find(serviceName);
        if (iterator != subscribeMap.end()) {
            subscribeMap.erase(iterator);
        }
    }

    unique_ptr<vector<string>> DemoNamingServer::lookup(const string_view &service) {
        return unique_ptr<vector<string>>();
    }
}
