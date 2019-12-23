//
// Created by dear on 19-12-22.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>
#include "conn_channel.h"
#include <errno.h>

void ConnChannel::OnEvent(int mask) {

    if (mask & Event_READABLE) {
        char buf[2048];
        size_t len = 2048;
        ssize_t i = read(fd, buf, len);

        std::cout << "1_size:" << i << std::endl;
        i = read(fd, buf, len);
        std::cout << "1_size:" << i << std::endl;
        if (i == -1 && errno == EAGAIN) {
            std::cout << "读到末尾了" << std::endl;
        }
        const char str[] = "读到了";
        write(fd, str, sizeof(str));
    }

}

static int setTcpNoDelay(int fd, int val) {
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        return -1;
    }
    return 0;
}

static int setTcpKeepAlive(int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        return -1;
    }
    return 0;
}

int ConnChannel::Init() {
    MarkNonBlock();
    setTcpNoDelay(fd, 1);
    setTcpKeepAlive(fd);
    return 0;
}
