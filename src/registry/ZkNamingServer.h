//
// Created by dear on 3/13/20.
//

#ifndef SKYNET_ZKNAMINGSERVER_H
#define SKYNET_ZKNAMINGSERVER_H

#include "naming_server.h"
#include <zookeeper/zookeeper.h>
#include <boost/unordered_map.hpp>
#include <mutex>

namespace sn {
    struct ZkConfig {
        string ipHosts;
        int recvTimeout;
        string zkNamespace;
        string registerStr;

        ZkConfig(const string &ipHosts, int recvTimeout, const string &zkNamespace,
                 const string &registerStr) : ipHosts(ipHosts), recvTimeout(recvTimeout),
                                              zkNamespace(zkNamespace), registerStr(registerStr) {}

        ZkConfig() = default;

        ZkConfig(const ZkConfig &oth)
                : ipHosts(oth.ipHosts),
                  recvTimeout(oth.recvTimeout),
                  zkNamespace(oth.zkNamespace), registerStr(oth.zkNamespace) {
        }

    };


    class ZkNamingServer : public NamingServer {
    private:
        struct WatchContext {
            SubscribeFunc func;
            void *param;
            string path;
            string service;
            vector <string> val;

            WatchContext(SubscribeFunc func, void *param, string &path, const string_view &service)
                    : func(func), param(param), path(path), service(service) {}
        };


        static void watcherZookeeper(zhandle_t *zh, int type,
                                     int state, const char *path, void *watcherCtx);

        static void watcherService(zhandle_t *zh, int type,
                                   int state, const char *path, void *watcherCtx);

    private:
        zhandle_t *handle;
        clientid_t clientId{};
        ZkConfig config;
        boost::unordered_map<string, shared_ptr < WatchContext>> subscribeMap;
        mutex subscribeLock;

    public:
        explicit ZkNamingServer(const ZkConfig &config);

        ~ZkNamingServer();

        unique_ptr <vector<string>> lookup(const string_view &service) override;

        unique_ptr <vector<string>> subscribe(const string_view &serviceName, SubscribeFunc func, void *param) override;

        void unsubscribe(const string_view &serviceName) override;

        void registerService(const string_view &serviceName) override;

        void unregisterService(const string_view &service) override;

    private:
        void createNode(const string_view &serviceName) const;

        void removeNode(const string_view &view);
    };

}


#endif //SKYNET_ZKNAMINGSERVER_H
