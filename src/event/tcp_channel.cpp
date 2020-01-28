//
// Created by dear on 20-1-27.
//

#include "tcp_channel.h"

namespace sn {
    TcpChannel::TcpChannel(int fd, ip_t ip, int port) : ConnChannel(fd), raddr(ip, port) {
        get_local_side(fd, &laddr);
        LOG(INFO) << raddr << " --> " << laddr << " TcpChannel start";
    }

    TcpChannel::~TcpChannel() {
        LOG(INFO) << raddr << " --> " << laddr << " TcpChannel stop";

    }
}

