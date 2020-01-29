//
// Created by dear on 19-12-22.
//

#include <sys/socket.h>
#include "socket_channel.h"
#include <arpa/inet.h>
#include "conn_channel.h"
#include "tcp_channel.h"
#include "shake_hands_channel.h"

namespace sn {
    using namespace google;

    void SocketChannel::doClose() {
        close(fd);
    }

    int SocketChannel::doRead() {
        sockaddr_storage sa = {0};
        socklen_t in_len = sizeof(in_addr);
        int newFd = accept(fd, reinterpret_cast<sockaddr *>(&sa), &in_len);
        ConnChannel *channel;
        if (newFd != -1) {
            // 判断是否是IPV4还是IPV6
            if (sa.ss_family == AF_INET) {
                auto *s = (struct sockaddr_in *) &sa;
                // ip地址转化为字符串
                channel = new ShakeHandsChannel(newFd, getServiceRegistry());

            } else {
                LOG(ERROR) << "cannot support ipv6";
                close(fd);
                return 0;
            }

            int i = channel->Init();
            if (i < 0) {
                delete channel;
                return 0;
            }
            channel->AddEventLoop(dispatcher);
        }
        return 0;
    }

    int SocketChannel::doWrite() {
        return 0;
    }

}
