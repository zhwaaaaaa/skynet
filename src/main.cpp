#include <iostream>
#include <event/event_dispatcher.h>
#include <event/socket_channel.h>
#include <util/buffer.h>
#include <registry/service_registry.h>

using namespace sn;

int main() {

    std::cout << "SegmentRef " << sizeof(SegmentRef) << std::endl;

    EventDispatcher dispatcher(100);

    EndPoint listenAddr;

    str2endpoint(nullptr, 9999, &listenAddr);

    const int tcpListen = tcp_listen(listenAddr);
    if (tcpListen < 0) {
        LOG(FATAL) << "ERROR LISTEN " << endpoint2str(listenAddr) << "," << strerror(errno);
        return -1;
    }

    LOG(INFO) << "LISTEN AT " << endpoint2str(listenAddr);
    SocketChannel sc(listenAddr, tcpListen);

    int i = sc.Init();
    if (i < 0) {
        perror("出错了");
        return -1;
    }
    sc.AddEventLoop(&dispatcher);

    createServiceRegistry(&dispatcher);

    dispatcher.RunLoop();

    return 0;
}