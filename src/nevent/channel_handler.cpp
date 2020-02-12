//
// Created by dear on 2020/2/4.
//

#include "channel_handler.h"
#include <glog/logging.h>

#include <utility>
#include <util/func_time.h>

namespace sn {
    ChannelHandler::ChannelHandler(shared_ptr<Channel> ch) : ch(std::move(ch)) {}

    void ChannelHandler::onChannelClosed(uv_handle_t *handle) {
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        delete handler;
    }

    void ChannelHandler::onMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        auto start = currentTimeUs();
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        handler->onMemoryRequired(suggested_size, buf);
        auto cost = currentTimeUs() - start;
        if (cost > 2) {
            LOG(INFO) << "onMemoryAlloc Time const " << cost;
        }
    }

    void ChannelHandler::onMessageArrived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {

        ChannelHandler *handler = static_cast<ChannelHandler *>(stream->data);
        if (nread > 0) {
            if (handler->onMessage(buf, nread) == 0) {
            }
            // close
        } else if (nread == UV_EOF) {
            handler->onClose(buf);
        } else {
            LOG(ERROR) << "Error onMessageArrived:" << uv_strerror(nread);
            handler->onError(buf);
        }

    }


}
