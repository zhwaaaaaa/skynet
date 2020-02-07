//
// Created by dear on 2020/2/3.
//

#ifndef SKYNET_CHANNEL_H
#define SKYNET_CHANNEL_H

#include <uv.h>
#include <util/buffer.h>
#include <util/endpoint.h>
#include <strings.h>

namespace sn {
    class Loopable {
    public:
        virtual void addToLoop(uv_loop_t *loop) = 0;
    };

    struct WritedData {
        uint32_t len;
    };

    class Channel : public Loopable {
    public:
        virtual uint32_t channelId() = 0;

        virtual int writeMsg(Buffer *buffer, uint32_t firstOffset, uint32_t len) = 0;

        virtual void close() = 0;

        virtual ~Channel() = default;
    };


}


#endif //SKYNET_CHANNEL_H
