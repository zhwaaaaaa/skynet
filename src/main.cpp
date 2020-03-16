#include <nevent/tcp_listener.h>
#include <nevent/ResponseHandler.h>
#include <nevent/shake_hands_handler.h>
#include <event/server.h>
#include <nevent/RequestHandler.h>
#include <registry/ZkNamingServer.h>

using namespace sn;

int main() {
    ZkConfig config(
            "ubuntu:2181",
            5000,
            "",
            "skynet",
            "192.168.176.128:9999"
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
    return 0;
}

