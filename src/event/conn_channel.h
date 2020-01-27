//
// Created by dear on 19-12-22.
//

#ifndef MESHER_CONN_CHANNEL_H
#define MESHER_CONN_CHANNEL_H

#include "channel.h"
#include <string>
#include <util/buffer.h>

namespace sn {
    enum Action {
        NEXT,
        SUCCESS,
        ERROR
    };

    class ConnChannel;

    typedef Action *(writeFunc)(ConnChannel *ch, void *data);

    struct WriteEvt {
        void *data;
        writeFunc func;
        WriteEvt *nextEvt;
    };

    struct Header {
        const Segment<uint8_t> *servName;
        const Segment<uint8_t> *methodName;
        uint32_t *reqId;
        uint32_t *clientId;
        uint32_t *serverId;
        uint32_t bodyLen;

    };

    class ConnChannel : public Channel {
        enum DECODE_STATUS {
            READING_HEAD,
            READING_BODY
        };
    private:
        Buffer buf;
        DECODE_STATUS decodeStatus;

    public:
        explicit ConnChannel(int fd) : Channel(),
                                       buf(16384),
                                       decodeStatus(READING_HEAD) {
            this->fd = fd;
        }

        int Init() override;

    protected:
        int doRead() override;

        int doWrite() override;

        void doClose() override;

        int tryDecodeBuf();

    private:
        int decodeHead(Segment<uint8_t> *head, Header *header);
    };
}

#endif //MESHER_CONN_CHANNEL_H
