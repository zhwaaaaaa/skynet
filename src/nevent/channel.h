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

        /**
         * 返回
         * @param buffer 数据buffer链表头
         * @param firstOffset 数据链表头第一个buffer偏移量
         * @param len 数据长度
         * @return >= 0 写成功、 -1 出错但是buffer还没被污染可自行处理， <-1 buffer已经被回收掉了
         */
        virtual int writeMsg(Buffer *buffer, uint32_t firstOffset, uint32_t len) = 0;

        virtual void close() = 0;

        virtual ~Channel() = default;
    };


}


#endif //SKYNET_CHANNEL_H
