#include <nevent/tcp_listener.h>
#include <nevent/ResponseHandler.h>
#include <nevent/shake_hands_handler.h>
#include <event/server.h>

using namespace sn;

int main() {
    TcpListener<ServerShakeHandsHandler> serverAppListener(EndPoint(IP_ANY, 9998));
    Server server;
    server.addLoopable(serverAppListener);
    TcpListener<ServerReqHandler> serverTransferListener(EndPoint(IP_ANY, 9999));
    server.addLoopable(serverTransferListener);
    server.start(true);

    Client client;
    TcpListener<ClientShakeHandsHandler> clientAppListener(EndPoint(IP_ANY, 9997));
    client.addLoopable(clientAppListener);
    client.start();
    return 0;
}

