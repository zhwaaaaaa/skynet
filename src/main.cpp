#include <nevent/tcp_listener.h>
#include <nevent/shake_hands_handler.h>
#include <event/server.h>
#include <nevent/RequestHandler.h>
#include <registry/ZkNamingServer.h>

#include <gflags/gflags.h>

using namespace sn;

static bool ValidateRegistry_register_ip_host(const char *flagname, string &val) {
    cout << val << endl;
}


DEFINE_string(registry_register_ip_host, "", "注册到zookeeper的ip和host");
DEFINE_string(registry_zookeeper_ip_hosts, "localhost:2181", "zookeeper的ip:host");
DEFINE_string(registry_zookeeper_namespace, "skynet", "zookeeper使用命名空间");
DEFINE_int32(registry_zookeeper_receive_timeout, 5000, "zookeeper的receive timeout");
DEFINE_int32(transfer_port, 9999, "监听的端口");
DEFINE_string(transfer_host, "0.0.0.0", "绑定的ip地址");
DEFINE_string(provider, "tcp://0.0.0.0:9998", "服务提供者连接的地址");
DEFINE_string(consumer, "tcp://0.0.0.0:9997", "服务消费者连接的地址");

int main(int argc, char *argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);


    ZkConfig config(FLAGS_registry_zookeeper_ip_hosts,
                    FLAGS_registry_zookeeper_receive_timeout,
                    FLAGS_registry_zookeeper_namespace,
                    FLAGS_registry_register_ip_host
    );

    ZkNamingServer zkNamingServer(config);

    TcpListener<ServerShakeHandsHandler> serverAppListener(EndPoint(IP_ANY, 9998));
    Server server(zkNamingServer);

    server.addLoopable(serverAppListener);
    TcpListener<ServerReqHandler> serverTransferListener(EndPoint(IP_ANY, 9999));
    server.addLoopable(serverTransferListener);
    server.start(true);

    Client client(zkNamingServer);
    TcpListener<ClientShakeHandsHandler> clientAppListener(EndPoint(IP_ANY, 9997));
    client.addLoopable(clientAppListener);
    client.start();

    google::ShutDownCommandLineFlags();
    return 0;
}

