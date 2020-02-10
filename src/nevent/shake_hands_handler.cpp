//
// Created by dear on 2020/2/10.
//

#include "shake_hands_handler.h"
#include "transfer_handler.h"

namespace sn {

    ClientShakeHandsHandler::ClientShakeHandsHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), shakeComplete(false), readBytes(0), firstBuffer(nullptr), lastBuffer(nullptr),
              lastBufferOffset(0), serviceSize(0) {
        bzero(tmpServ, sizeof(tmpServ));
    }

    void ClientShakeHandsHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {
        if (!firstBuffer) {
            firstBuffer = lastBuffer = ByteBuf::alloc();
        }

        if (lastBufferOffset == BUFFER_BUF_LEN) {
            lastBuffer->next = ByteBuf::alloc();
            lastBuffer = lastBuffer->next;
            lastBufferOffset = 0;
        }

        buf->len = BUFFER_BUF_LEN - lastBufferOffset;
        buf->base = lastBuffer->buf + lastBufferOffset;
    }

    int ClientShakeHandsHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        readBytes += nread;
        lastBufferOffset += nread;

        if (readBytes < shakePkgLen || (!shakePkgLen && readBytes < 6)) {
            // 没有读满一个包，或者包长度没解析出来并且现在还不够解析
            return 0;
        }

        if (!shakePkgLen) {
            ShakeHandsHeader *header = reinterpret_cast<ShakeHandsHeader *>(firstBuffer->buf);
            shakePkgLen = CONVERT_VAL_32(header->len) + 4;
            if (SERVICE_TYPE_CLIENT != header->shakeType) {
                ch->close();
                return -1;
            }
            serviceSize = CONVERT_VAL_16(header->serviceSize);
        }
        if (readBytes == shakePkgLen) {
            LOG(INFO) << "握手包接收完毕,serviceSize:" << serviceSize;
            // TODO subscribe to naming server and connect to remote server
            auto pHandler = ch->replaceHandler(new ClientAppHandler(ch));
            assert(pHandler == this);
            delete pHandler;// 自杀
            return 0;
        } else if (readBytes > shakePkgLen) {
            // 握手成功之前，已经发数据过来了，关闭
            ch->close();
            return -1;
        }

        return 0;
    }

    void ClientShakeHandsHandler::onClose(const uv_buf_t *buf) {
        if (firstBuffer) {
            ByteBuf::recycleWrited(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
        ch->close();
    }

    void ClientShakeHandsHandler::onError(const uv_buf_t *buf) {
        if (firstBuffer) {
            ByteBuf::recycleWrited(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
        ch->close();
    }

    ClientShakeHandsHandler::~ClientShakeHandsHandler() {
        if (firstBuffer) {
            ByteBuf::recycleWrited(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
    }

}
