//
// Created by dear on 20-1-29.
//

#include "naming_server.h"

namespace sn {

    void DemoNamingServer::subscribe(const string_view &serviceName, SubscribeFunc func, void *param) {

        subscribeMap.insert(make_pair(serviceName, FuncWithParam({func, param})));
        EndPoint ep;
        str2endpoint("127.0.0.1:9999", &ep);
        func(serviceName, {}, true, param);
    }

    void DemoNamingServer::unsubscribe(const string_view &serviceName) {
        auto iterator = subscribeMap.find(serviceName);
        if (iterator != subscribeMap.end()) {
            const auto &fwp = iterator->second;
            EndPoint ep;
            str2endpoint("127.0.0.1:9999", &ep);
            fwp.func(serviceName, {ep}, false, fwp.param);
        }
    }
}
