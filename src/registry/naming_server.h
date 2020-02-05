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


    typedef void (*SubscribeFunc)(const string &, const vector<EndPoint> &, bool hashNext, void *param);

    class NamingServer {
    public:
        virtual ~NamingServer() = default;

        virtual void subscribe(const string &serviceName, SubscribeFunc func, void *param) = 0;

        virtual void unsubscribe(const string &serviceName) = 0;

    };


    class DemoNamingServer : public NamingServer {
    private:

        struct FuncWithParam {
            SubscribeFunc func;
            void *param;
        };

    private:
        hash_map<string, FuncWithParam> subscribeMap;

    public:
        void subscribe(const string &serviceName, SubscribeFunc func, void *param) override;

        void unsubscribe(const string &serviceName) override;
    };
}


#endif //SKYNET_NAMING_SERVER_H
