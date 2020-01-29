//
// Created by dear on 20-1-29.
//

#include "naming_server.h"

namespace sn {


    const std::vector<EndPoint> DemoNamingServer::find(const std::string &serviceName) {
        EndPoint ep;
        str2endpoint("127.0.0.1:9999", &ep);
        return {ep};
    }

    int DemoNamingServer::subscribe(const std::string &serviceName, SubscribeFunc fun) {
        return 0;
    }

    int DemoNamingServer::unsubscribe(const string &serviceName) {
        return 0;
    }
}
