#include <iostream>
#include <event/reactor.h>
#include <nevent/tcp_listener.h>

using namespace sn;

int main() {
    Reactor reactor1;
    TcpListener<ClientAppHandler> listener1(EndPoint(IP_ANY, 9999));
    reactor1.addLoopable(listener1);
    reactor1.start(true);


    Reactor reactor2;
    TcpListener<ClientAppHandler> listener2(EndPoint(IP_ANY, 9998));
    reactor2.addLoopable(listener2);
    reactor2.start();
    return 0;
}

