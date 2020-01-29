//
// Created by dear on 20-1-29.
//

#ifndef SKYNET_NAMING_SERVER_H
#define SKYNET_NAMING_SERVER_H

#include <util/endpoint.h>
#include <vector>
#include <util/buffer.h>
#include <util/HASH_MAP_HPP.h>

namespace sn {
    using namespace std;
    using namespace __gnu_cxx;

    typedef int(*SubscribeFunc)(const string &, const vector<EndPoint> &);


    class NamingServer {
    private:
    public:
        virtual ~NamingServer() = default;

        virtual const vector<EndPoint> find(const string &serviceName)=0;

        virtual int subscribe(const string &serviceName, SubscribeFunc fun)=0;

        virtual int unsubscribe(const string &serviceName)=0;

    };


    class DemoNamingServer : public NamingServer {

    private:
        struct FunctionWrap {
            SubscribeFunc func;

            FunctionWrap() = default;

            FunctionWrap(SubscribeFunc func) noexcept : func(func) {}

            FunctionWrap(const FunctionWrap &wrap) noexcept = default;

//            FunctionWrap(const FunctionWrap &&wrap)noexcept = default;

            FunctionWrap &operator=(const FunctionWrap &) = default;
        };

        hash_map<string, FunctionWrap> subscribeMap;

    public:
        const vector<EndPoint> find(const string &serviceName) override;

        int subscribe(const string &serviceName, SubscribeFunc fun) override;

        int unsubscribe(const string &serviceName) override;
    };
}


#endif //SKYNET_NAMING_SERVER_H
