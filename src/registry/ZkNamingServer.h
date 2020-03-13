//
// Created by dear on 3/13/20.
//

#ifndef SKYNET_ZKNAMINGSERVER_H
#define SKYNET_ZKNAMINGSERVER_H

#include "naming_server.h"
#include <zookeeper/zookeeper.h>

namespace sn {
    struct ZkConfig {
        string ipHosts;
        int recvTimeout;
        string password;
        string zkNamespace;
        string registerStr;

        ZkConfig() = default;

        explicit ZkConfig(const ZkConfig &oth)
                : ipHosts(oth.ipHosts),
                  recvTimeout(oth.recvTimeout), password(oth.password),
                  zkNamespace(oth.zkNamespace), registerStr(oth.zkNamespace) {
        }

    };

    class ZkNamingServer : public NamingServer {
    private:
        static void watcherZookeeper(zhandle_t *zh, int type,
                                     int state, const char *path, void *watcherCtx);
        static void watcherService(zhandle_t *zh, int type,
                                     int state, const char *path, void *watcherCtx);

    private:
        zhandle_t *handle;
        clientid_t clientId{};
        ZkConfig config;

    public:
        explicit ZkNamingServer(const ZkConfig &config);

        ~ZkNamingServer();

        unique_ptr <vector<string>> lookup(const string_view &service) override;

        unique_ptr<vector<string>> subscribe(const string_view &serviceName, SubscribeFunc func, void *param) override;

        void unsubscribe(const string_view &serviceName) override;

        void registerService(const string_view &serviceName) override;

        void unregisterService(const string_view &service) override;

    private:
        void createNode(const string_view &serviceName) const;

        void removeNode(const string_view &view);
    };

}


#endif //SKYNET_ZKNAMINGSERVER_H
