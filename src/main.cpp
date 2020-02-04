#include <iostream>
#include <event/reactor.h>
#include <nevent/tcp_listener.h>

using namespace sn;

int main() {

    Reactor reactor;

    TcpListener<ClientAppHandler> listener(EndPoint(IP_ANY, 9999));
    reactor.addLoopable(listener);
    reactor.start();

    return 0;
}