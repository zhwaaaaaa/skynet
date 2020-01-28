#include <iostream>
#include <event/event_dispatcher.h>
#include <event/socket_channel.h>
#include <util/buffer.h>

using namespace sn;

int main() {

    std::cout << "SegmentRef " << sizeof(SegmentRef) << std::endl;

    EventDispatcher dispatcher(100);

    SocketChannel sc("0.0.0.0", 9999);

    int i = sc.Init();
    if (i < 0) {
        perror("出错了");
        return -1;
    }
    sc.AddEventLoop(&dispatcher);

    dispatcher.Select(-1);

    return 0;
}