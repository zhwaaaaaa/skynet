#include <event/reactor.h>
#include <nevent/tcp_listener.h>
#include <nevent/transfer_handler.h>
#include <nevent/shake_hands_handler.h>

using namespace sn;

int main() {

    Reactor reactor1;
    TcpListener<ClientShakeHandsHandler> listener1(EndPoint(IP_ANY, 9999));
    reactor1.addLoopable(listener1);
    reactor1.start();
    return 0;
}

