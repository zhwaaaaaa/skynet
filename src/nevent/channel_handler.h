//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_CHANNEL_HANDLER_H
#define SKYNET_CHANNEL_HANDLER_H

#include "channel.h"
#include <util/byte_buf.h>
#include <memory>

namespace sn {
    using namespace std;
    using ChannelPtr = std::shared_ptr<Channel>;

    class ChannelHandler {
    protected:
        const ChannelPtr ch;
    public:
        explicit ChannelHandler(shared_ptr<Channel> ch);

        static void onChannelClosed(uv_handle_t *handle);

        static void onMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        static void onMessageArrived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

    public:
        virtual ~ChannelHandler() = default;


    protected:
        virtual void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) = 0;

        virtual int onMessage(const uv_buf_t *buf, ssize_t nread) = 0;

        virtual void onClose(const uv_buf_t *buf) = 0;

        virtual void onError(const uv_buf_t *buf) = 0;

    };


    class ClientTransferHandler : public ChannelHandler {
    public:
        explicit ClientTransferHandler(const shared_ptr<Channel> &ch);

    private:
        void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) override;

        int onMessage(const uv_buf_t *buf, ssize_t nread) override;

        void onClose(const uv_buf_t *buf) override;

        void onError(const uv_buf_t *buf) override;
    };

}


#endif //SKYNET_CHANNEL_HANDLER_H
