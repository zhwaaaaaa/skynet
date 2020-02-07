//
// Created by dear on 2020/2/6.
//

#include "transfer_handler.h"


namespace sn {


    TransferHandler::TransferHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch),lastReadBuffer(nullptr), bufferUsed(0), headerLen(0), outCh(nullptr) {
    }

    void TransferHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {
        if (lastReadBuffer) {
            buf->len = BUFFER_BUF_LEN - bufferUsed;
            buf->base = lastReadBuffer->buf + bufferUsed;
        } else {
            auto pBuffer = ByteBuf::alloc();
        }
    }

    int TransferHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        auto outChannel = outCh.get();
        if(outChannel){
        }
        if(lastReadBuffer){

        }


        return 0;
    }

}
