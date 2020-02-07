//
// Created by dear on 2020/2/4.
//

#include "channel_handler.h"
#include <glog/logging.h>

#include <utility>

namespace sn {
    void ChannelHandler::onChannelClosed(uv_handle_t *handle) {
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        delete handler;
    }

    void ChannelHandler::onMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        handler->onMemoryRequired(suggested_size, buf);
    }

    void ChannelHandler::onMessageArrived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
        ChannelHandler *handler = static_cast<ChannelHandler *>(stream->data);
        if (nread > 0) {
            if (handler->onMessage(buf, nread) == 0) {
                return;
            }
            // close
        } else if (nread == UV_EOF) {
            handler->onClose(buf);
        } else {
            handler->onError(buf);
        }
    }

    ChannelHandler::ChannelHandler(shared_ptr<Channel> ch) : ch(std::move(ch)) {}


    ClientAppHandler::ClientAppHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), valid(false), readedBytes(0), requireBytes(0), lastReadBuffer(nullptr),
              readedOffset(0), decodeOffset(0) {
    }

    void ClientAppHandler::onMemoryRequired(size_t, uv_buf_t *buf) {
        if (readedOffset) {
            buf->len = BUFFER_BUF_LEN - readedOffset;
            buf->base = lastReadBuffer->buf + readedOffset;
        } else {
            auto buffer = byteBuf.alloc();
            buf->base = buffer->buf;
            buf->len = BUFFER_BUF_LEN;
        }
    }

    int ClientAppHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        if (packageLen) {

        }


        return 0;
    }

    void ClientAppHandler::onClose(const uv_buf_t *buf) {
        LOG(INFO) << ch.get()->channelId() << " ClientAppHandler::onClose";
    }

    void ClientAppHandler::onError(const uv_buf_t *buf) {
        LOG(INFO) << ch.get()->channelId() << " ClientAppHandler::onError";

    }

    ClientTransferHandler::ClientTransferHandler(shared_ptr<Channel> &ch) : ChannelHandler(ch) {
    }

    void ClientTransferHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {

    }

    int ClientTransferHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        return 0;
    }

    void ClientTransferHandler::onClose(const uv_buf_t *buf) {
    }

    void ClientTransferHandler::onError(const uv_buf_t *buf) {
    }


}
