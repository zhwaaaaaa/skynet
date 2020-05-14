#include <nevent/tcp_listener.h>
#include <nevent/shake_hands_handler.h>
#include <event/server.h>
#include <nevent/RequestHandler.h>
#include <registry/naming_server.h>

#include <gflags/gflags.h>

using namespace sn;

DEFINE_int32(transfer_port, 9999, "监听的端口");
DEFINE_string(transfer_host, "0.0.0.0", "绑定的ip地址");
DEFINE_string(provider, "tcp://0.0.0.0:9998", "服务提供者连接的地址");
DEFINE_string(consumer, "tcp://0.0.0.0:9997", "服务消费者连接的地址");

int main(int argc, char *argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    unique_ptr<NamingServer> nsPtr = createNamingServer();

    TcpListener<ServerShakeHandsHandler> serverAppListener(EndPoint(IP_ANY, 9998));
    Server server(*nsPtr);

    server.addLoopable(serverAppListener);
    TcpListener<ServerReqHandler> serverTransferListener(EndPoint(IP_ANY, 9999));
    server.addLoopable(serverTransferListener);
    server.start(true);

    Client client(*nsPtr);
    TcpListener<ClientShakeHandsHandler> clientAppListener(EndPoint(IP_ANY, 9997));
    client.addLoopable(clientAppListener);
    client.start();

    google::ShutDownCommandLineFlags();
    return 0;
}

