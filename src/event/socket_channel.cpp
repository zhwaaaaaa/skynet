//
// Created by dear on 19-12-22.
//

#include <sys/socket.h>
#include "socket_channel.h"
#include <algorithm>
#include <netdb.h>
#include <arpa/inet.h>
#include "conn_channel.h"
#include "tcp_channel.h"
#include <glog/logging.h>

namespace sn {
    using namespace google;

    static int setReuseAddr(int fd) {
        int yes = 1;
        /* Make sure connection-intensive things like the redis benckmark
         * will be able to close/open sockets a zillion of times */
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            return -1;
        }
        return 0;
    }

    static int setReusePort(int fd) {
        int yes = 1;
        /* Make sure connection-intensive things like the redis benckmark
         * will be able to close/open sockets a zillion of times */
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) == -1) {
            return -1;
        }
        return 0;
    }

    int SocketChannel::Init() {

        std::string port_str = std::to_string(port);
        addrinfo *servinfo;
        if (getaddrinfo(host.data(), port_str.data(), nullptr, &servinfo) == -1) {
            return -1;
        }
        int s;
        addrinfo *p;
        for (p = servinfo; p != nullptr && p->ai_family == AF_INET; p = p->ai_next) {
            if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) > 0) {
                if (setReuseAddr(s) < 0) {
                    close(s);
                    return -1;
                }
                if (setReusePort(s) < 0) {
                    close(s);
                    return -1;
                }
                if (bind(s, p->ai_addr, p->ai_addrlen) < 0) {
                    close(s);
                    return -1;
                }
                if (listen(s, backlog) != 0) {
                    close(s);
                    return -1;
                }
                this->fd = s;
                LOG(INFO) << "LISTENING AT " << EndPoint(*reinterpret_cast<sockaddr_in *>(p->ai_addr));
                return 0;
            }
        }
        return -1;
    }

    void SocketChannel::doClose() {

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
                channel = new TcpChannel(newFd, s->sin_addr, ntohs(s->sin_port));

            } else {
                LOG(ERROR) << "cannot support ipv6";
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
