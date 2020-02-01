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

#define TRY_WRITE 0
#define CAN_WRITE 1
#define WRITE_ERR 2

    typedef Action (*writeFunc)(int status, ConnChannel *ch, void *param);

    struct WriteEvt {
        void *param;
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
    protected:
        Buffer buf;
    private:
        DECODE_STATUS decodeStatus;
        WriteEvt *headEvt;
        WriteEvt *tailEvt;

    public:
        explicit ConnChannel(int fd);

        virtual int AddWriteEvt(writeFunc func, void *param) final;

    protected:
        int doRead() override;

        int doWrite() override;

        virtual int tryDecodeBuf();

    private:
        int decodeHead(Segment<uint8_t> *head, Header *header);
    };
}

#endif //MESHER_CONN_CHANNEL_H
