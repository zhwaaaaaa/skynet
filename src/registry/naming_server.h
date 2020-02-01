//
// Created by dear on 20-1-29.
//

#ifndef SKYNET_NAMING_SERVER_H
#define SKYNET_NAMING_SERVER_H

#include <util/endpoint.h>
#include <vector>
#include <util/buffer.h>
#include <util/HASH_MAP_HPP.h>
#include <util/func_time.h>

namespace sn {
    using namespace std;
    using namespace __gnu_cxx;


    typedef int (*SubscribeFunc)(const string &, const vector<EndPoint> &);

    class NamingServer {
    public:
        virtual ~NamingServer() = default;

        virtual const vector<EndPoint> find(const string &serviceName)=0;

        virtual int subscribe(const string &serviceName, SubscribeFunc func) =0;

        virtual int unsubscribe(const string &serviceName)=0;

    };


    class DemoNamingServer : public NamingServer {

    private:
        hash_map<string, SubscribeFunc> subscribeMap;

    public:
        const vector<EndPoint> find(const string &serviceName) override;

        int subscribe(const string &serviceName, SubscribeFunc fun) override;

        int unsubscribe(const string &serviceName) override;
    };
}


#endif //SKYNET_NAMING_SERVER_H
