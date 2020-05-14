//
// Created by eleme on 2020/5/14.
//
#include <gflags/gflags.h>

#include <memory>
#include "naming_server.h"

#ifdef USE_ZK
#include "ZkNamingServer.h"

DEFINE_string(registry_register_ip_host, "", "注册到zookeeper的ip和host");
DEFINE_string(registry_zookeeper_ip_hosts, "localhost:2181", "zookeeper的ip:host");
DEFINE_string(registry_zookeeper_namespace, "skynet", "zookeeper使用命名空间");
DEFINE_int32(registry_zookeeper_receive_timeout, 5000, "zookeeper的receive timeout");
#else
DEFINE_string(registry_fix_ip_hosts, "127.0.0.1:9999", "固定的ip:port");
//static std::string FLAGS_registry_fix_ip_hosts = "localhost:9999";
namespace internal {
    using namespace std;
    using namespace sn;

    class FixEndpointNamingServer : public NamingServer {
    public:
        std::unique_ptr<vector<std::string>> lookup(const std::string_view &service) {
            vector<string> v = {FLAGS_registry_fix_ip_hosts};
            return std::make_unique<vector<string>>(std::move(v));
        }

        unique_ptr<vector<string>> subscribe(const string_view &serviceName, SubscribeFunc func, void *param) {
            vector<string> v = {FLAGS_registry_fix_ip_hosts};
            return std::make_unique<vector<string>>(std::move(v));
        }

        void unsubscribe(const string_view &serviceName) {

        }

        void registerService(const string_view &service) {

        }

        void unregisterService(const string_view &service) {

        }
    };
}

#endif
namespace sn {
    std::unique_ptr<NamingServer> createNamingServer() {
#ifdef USE_ZK

        ZkConfig config(FLAGS_registry_zookeeper_ip_hosts,
                        FLAGS_registry_zookeeper_receive_timeout,
                        FLAGS_registry_zookeeper_namespace,
                        FLAGS_registry_register_ip_host
        );

        ZkNamingServer *p = new ZkNamingServer(config);
        return std::unique_ptr<NamingServer>(static_cast<NamingServer *>(p));
#else
        return unique_ptr<NamingServer>(new internal::FixEndpointNamingServer);
#endif
    }
}


