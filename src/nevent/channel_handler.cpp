//
// Created by dear on 2020/2/4.
//

#include "channel_handler.h"
#include <glog/logging.h>
#include <util/func_time.h>

namespace sn {
    ChannelHandler::ChannelHandler(shared_ptr<Channel> ch) : ch(std::move(ch)) {}

    void ChannelHandler::onChannelClosed(uv_handle_t *handle) {
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        delete handler;
        BlockPool::report();
    }

    void ChannelHandler::onMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
#if DEBUG
        auto start = currentTimeUs();
#endif
        ChannelHandler *handler = static_cast<ChannelHandler *>(handle->data);
        handler->ioBuf.writePtr(&buf->base, &buf->len);
#if DEBUG
        auto cost = currentTimeUs() - start;
        if (cost > 2) {
            LOG(INFO) << "onMemoryAlloc Time const " << cost;
        }
#endif
    }

    void ChannelHandler::onMessageArrived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
#if DEBUG
        auto start = currentTimeUs();
#endif
        ChannelHandler *handler = static_cast<ChannelHandler *>(stream->data);
        if (nread > 0) {
            handler->ioBuf.addSize(nread);
            if (handler->onRead(nread) != 0) {
                LOG(INFO) << "Close channel because found error onRead";
                handler->ch->close();
            }
            // close
        } else if (nread == UV_EOF) {
            LOG(ERROR) << "Close onMessageArrived:" << uv_strerror(nread);
            handler->onClose();
        } else {
            LOG(ERROR) << "Error onMessageArrived:" << uv_strerror(nread);
            handler->onError();
        }
#if DEBUG
        auto cost = currentTimeUs() - start;
        if (cost > 2) {
            LOG(INFO) << "onMessageArrived( "<< nread << ")Time const" << cost;
        }
#endif
    }

    int ChannelHandler::onRead(ssize_t nread) {
        int size = (int) ioBuf.getSize();
        while (size >= 5) {
            uint32_t len = CONVERT_VAL_32(ioBuf.read<uint32_t>(1)) + 5;
            if (size < len) {
                return 0;
            }

            IoBuf msg;
            ioBuf.popInto(msg, len);
            auto type = msg.readUint8();
            switch (type) {
                case MT_CONSUMER_SH:
                case MT_PROVIDER_SH:
                case MT_REQUEST:
                case MT_RESPONSE: {
                    auto i = onMessage(msg);
                    if (i != 0) {
                        return i;
                    }
                    break;
                }
                    /* case  MT_SH_RESP: not hit.*/
                case MT_HEARTBEAT_REQ: {
                    //return MT_HEARTBEAT_RESP
                    msg.modifyData<uint8_t>(MT_HEARTBEAT_RESP);
                    if (ch->writeMsg(msg) < 0) {
                        return -1;
                    }
                    break;
                }
                case MT_HEARTBEAT_RESP:
                    // do nothing
                    break;
                default:
                    LOG(WARNING) << "close channel because invalid message:" << type;
                    return -1;
            }
            size -= len;
        }

        return 0;
    }


}
