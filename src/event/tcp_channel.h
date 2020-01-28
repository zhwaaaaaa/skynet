//
// Created by dear on 20-1-27.
//

#ifndef SKYNET_TCP_CHANNEL_H
#define SKYNET_TCP_CHANNEL_H

#include <glog/logging.h>
#include "conn_channel.h"

namespace sn {
    class TcpChannel : public ConnChannel {
    public:
        TcpChannel(int fd, ip_t ip, int port);

        virtual ~TcpChannel();

        const EndPoint &remoteAddr() {
            return raddr;
        }

        const EndPoint &localAddr() {
            return laddr;
        }

    private:
        EndPoint raddr;
        EndPoint laddr;
    };
}


#endif //SKYNET_TCP_CHANNEL_H
