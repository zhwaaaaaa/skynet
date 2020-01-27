//
// Created by dear on 19-12-22.
//

#ifndef MESHER_CONN_CHANNEL_H
#define MESHER_CONN_CHANNEL_H

#include "channel.h"
#include <string>
#include <util/buffer.h>

namespace sn {

    class ConnChannel : public Channel {
        enum DECODE_STATUS {
            READING_HEAD,
            READING_BODY
        };
    private:
        EndPoint raddr;
        Buffer buf;
        DECODE_STATUS decodeStatus;

    public:
        explicit ConnChannel(int fd, ip_t ip, int port) : Channel(), raddr(ip, port), buf(16384),
                                                          decodeStatus(READING_HEAD) {
            this->fd = fd;
        }

        const EndPoint &RemoteAddr() const {
            return raddr;
        }

        virtual int Init() override;

        virtual void OnEvent(int mask) override;

    private:
        int decodeHead(Segment<uint8_t> *head);
    };
}

#endif //MESHER_CONN_CHANNEL_H
