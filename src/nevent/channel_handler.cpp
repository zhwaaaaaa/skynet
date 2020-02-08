//
// Created by dear on 2020/2/4.
//

#include "channel_handler.h"
#include <glog/logging.h>

#include <utility>

namespace sn {
    ChannelHandler::ChannelHandler(shared_ptr<Channel> ch) : ch(std::move(ch)) {}

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


    ClientTransferHandler::ClientTransferHandler(const shared_ptr<Channel> &ch) : ChannelHandler(ch) {
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
