//
// Created by dear on 2020/2/6.
//

#ifndef SKYNET_TRANSFER_HANDLER_H
#define SKYNET_TRANSFER_HANDLER_H


#include "channel_handler.h"
#include <bits/shared_ptr.h>

namespace sn {

    class TransferHandler : public ChannelHandler {
    private:
        size_t headerLen;
        std::shared_ptr<Channel> outCh;
        Buffer *lastReadBuffer;
        size_t bufferUsed;
    public:
        explicit TransferHandler(const shared_ptr<Channel> &ch);

    protected:
        void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) override;

        int onMessage(const uv_buf_t *buf, ssize_t nread) override;

        void onClose(const uv_buf_t *buf) override;

        void onError(const uv_buf_t *buf) override;

    };
}


#endif //SKYNET_TRANSFER_HANDLER_H
