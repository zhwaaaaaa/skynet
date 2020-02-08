//
// Created by dear on 2020/2/6.
//

#include "transfer_handler.h"
#include <glog/logging.h>

namespace sn {

    TransferHandler::TransferHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), lastReadBuffer(nullptr), firstBufferOffset(0), lastBufferUsed(0),
              packageLen(0), readPkgLen(0), firstReadBuffer(nullptr) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    void TransferHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {
        if (lastReadBuffer) {
            buf->len = BUFFER_BUF_LEN - lastBufferUsed;
            buf->base = lastReadBuffer->buf + lastBufferUsed;
        } else {
            lastReadBuffer = ByteBuf::alloc();
            buf->base = lastReadBuffer->buf;
            buf->len = BUFFER_BUF_LEN;
        }
    }

    int TransferHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        readPkgLen += nread;

        while (readPkgLen > packageLen) {
            ServiceNamePtr serviceName = reinterpret_cast<ServiceNamePtr>(firstReadBuffer->buf + firstBufferOffset);
            uint32_t serviceLen = serviceName->len;

            if (!packageLen) {
                if (serviceLen > MAX_SERVICE_LEN) {
                    // 不合法的地址
                    LOG(WARNING) << "超过MAX_SERVICE_LEN 238:" << serviceLen;
                    ch->close();
                    return -1;
                }

                if (readPkgLen <= serviceLen + REQ_HEADER_LEN) {
                    return 0;
                }

                uint32_t headLen = 1 + serviceLen + REQ_HEADER_LEN;
                if (firstBufferOffset + headLen > BUFFER_BUF_LEN) {
                    // 发生了header拆包。
                    // 上半个buffer的字节数
                    int topHalf = BUFFER_BUF_LEN - firstBufferOffset;
                    memcpy(tmpHead, serviceName, topHalf);
                    int secondHalf = headLen - topHalf;
                    memcpy(tmpHead + topHalf, firstReadBuffer->next->buf, secondHalf);
                    serviceName = reinterpret_cast<ServiceNamePtr>(tmpHead);

                    auto pHeader = serviceName->last<HeaderId>();
                    packageLen = headLen + pHeader->bodyLen;
                    setResponseChannelId(pHeader);

                    int headerBeforeClientIdLen = serviceLen + offsetof(HeaderId, clientId) + 1;
                    // [prevPkg][headerBeforeClientId][clientId][serverId][1:bodyType][4:bodyLen]
                    //          |              topHalf               |      secondHalf
                    //    firstBufferOffset                    firstBufferEnd
                    int firstBufferCopyLen = topHalf - headerBeforeClientIdLen;
                    if (firstBufferCopyLen > 0) {
                        //说明clientId,serverId 被拆开了，因为着两个字段有修改。所以需要复制两个buffer
                        //[prevPkg][headerBeforeClientId][clientId][serverId]
                        memcpy(firstReadBuffer->buf + firstBufferOffset + headerBeforeClientIdLen,
                               tmpHead + headerBeforeClientIdLen,
                               firstBufferCopyLen);//有可能超过8字节也没有关系。后边最多还有5个字节
                    }
                    // 因为最多复制8个字节 如果上一个buffer都复制完了，这个就不用复制了
                    if (firstBufferCopyLen < 8) {
                        if (firstBufferCopyLen < 0) {
                            // 上一个buffer一点都没复制
                            memcpy(firstReadBuffer->next->buf - firstBufferCopyLen,// 头
                                   tmpHead + headerBeforeClientIdLen,
                                   8);//至少有5个字节不用复制
                        } else {
                            memcpy(firstReadBuffer->next->buf,// 头
                                   tmpHead + headerBeforeClientIdLen,
                                   secondHalf - 5);//至少有5个字节不用复制
                        }

                    }
                } else {
                    auto pHeader = serviceName->last<HeaderId>();
                    packageLen = headLen + pHeader->bodyLen;
                    setResponseChannelId(pHeader);
                }
            } else {

                if (tmpHead[0]) {
                    //当前发生了拆包,header 在tmpHead中
                    serviceName = reinterpret_cast<ServiceNamePtr>(tmpHead);
                }

                lastBufferUsed = (packageLen - BUFFER_BUF_LEN + firstBufferOffset) % BUFFER_BUF_LEN;

                Channel *outCh = findOutCh(serviceName);

                uint32_t chId = outCh->channelId();
            }

        }


        return 0;
    }

    void TransferHandler::onClose(const uv_buf_t *buf) {
        ch->close();
    }

    void TransferHandler::onError(const uv_buf_t *buf) {
        ch->close();
    }


    ClientAppHandler::ClientAppHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), valid(false), readedBytes(0), requireBytes(0), lastReadBuffer(nullptr),
              readedOffset(0), decodeOffset(0) {
    }

    void ClientAppHandler::onMemoryRequired(size_t, uv_buf_t *buf) {
        if (readedOffset) {
            buf->len = BUFFER_BUF_LEN - readedOffset;
            buf->base = lastReadBuffer->buf + readedOffset;
        } else {
            auto buffer = ByteBuf::alloc();
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

}
