#include <nevent/tcp_listener.h>
#include <nevent/transfer_handler.h>
#include <nevent/shake_hands_handler.h>

using namespace sn;

int main() {

    Client client;
    TcpListener<ClientShakeHandsHandler> listener1(EndPoint(IP_ANY, 9999));
    client.addLoopable(listener1);
    client.start();
    return 0;
}

