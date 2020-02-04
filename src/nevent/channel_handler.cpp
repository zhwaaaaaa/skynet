//
// Created by dear on 2020/2/4.
//

#include "channel_handler.h"
#include <glog/logging.h>

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

        handler->ch->close();
    }

    ChannelHandler::ChannelHandler(Channel *ch) : ch(ch) {}

    ChannelHandler::~ChannelHandler() {
        delete ch;
    }

    ClientAppHandler::ClientAppHandler(Channel *ch) : ChannelHandler(ch) {
    }

    void ClientAppHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {
        LOG(INFO) << ch->channelId() << " ClientAppHandler::onMemoryRequired suggested_size:" << suggested_size;
        buf->base = static_cast<char *>(malloc(suggested_size));
        buf->len = suggested_size;
    }

    int ClientAppHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        LOG(INFO) << ch->channelId() << " ClientAppHandler::onMessage nread:" << nread;
        return 0;
    }

    void ClientAppHandler::onClose(const uv_buf_t *buf) {
        LOG(INFO) << ch->channelId() << " ClientAppHandler::onClose";
    }

    void ClientAppHandler::onError(const uv_buf_t *buf) {
        LOG(INFO) << ch->channelId() << " ClientAppHandler::onError";

    }

}
