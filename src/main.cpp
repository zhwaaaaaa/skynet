#include <iostream>
#include <event/reactor.h>
#include <nevent/tcp_listener.h>

using namespace sn;

int main() {

    uv_stream_t st;


    Reactor reactor1;
    TcpListener<ClientAppHandler> listener1(EndPoint(IP_ANY, 9999));
    reactor1.addLoopable(listener1);
    reactor1.start();
    return 0;
}

