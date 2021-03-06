//
// Created by dear on 20-1-29.
//

#ifndef SKYNET_NAMING_SERVER_H
#define SKYNET_NAMING_SERVER_H

#include <util/endpoint.h>
#include <vector>
#include <util/func_time.h>
#include <boost/unordered_map.hpp>
#include <memory>

namespace sn {
    using namespace std;

    typedef void (*SubscribeFunc)(const string_view &service, const vector<string> &val, void *param);

    class NamingServer {
    public:
        virtual unique_ptr<vector<string>> lookup(const string_view &service) = 0;

        virtual unique_ptr<vector<string>>
        subscribe(const string_view &serviceName, SubscribeFunc func, void *param) = 0;

        virtual void unsubscribe(const string_view &serviceName) = 0;

        virtual void registerService(const string_view &service) = 0;

        virtual void unregisterService(const string_view &service) = 0;

    };

    std::unique_ptr<NamingServer> createNamingServer();
}


#endif //SKYNET_NAMING_SERVER_H
