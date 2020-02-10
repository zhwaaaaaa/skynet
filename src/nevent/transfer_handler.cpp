//
// Created by dear on 2020/2/6.
//

#include "transfer_handler.h"
#include <glog/logging.h>
#include <thread/thread.h>
#include <event/client.h>

namespace sn {

    RequestHandler::RequestHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), lastReadBuffer(nullptr), firstBufferOffset(0), lastBufferUsed(0),
              packageLen(0), readPkgLen(0), firstReadBuffer(nullptr) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    void RequestHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {

        if (readPkgLen) {
            assert(lastReadBuffer && firstReadBuffer);
            if (lastBufferUsed == BUFFER_BUF_LEN) {
                // 上一个buffer已经读满了。并且还有字节没解析。所以要申请一个新buffer，连接上一个buffer
                Buffer *buffer = ByteBuf::alloc();
                lastReadBuffer->next = buffer;
                lastReadBuffer = buffer;
                lastBufferUsed = 0;
            }
        } else {
            assert(!firstReadBuffer);
            if (!lastReadBuffer) { // first will hit
                lastReadBuffer = ByteBuf::alloc();
            }
            // 包已经解析完了。并且这个包也已经快满了，没有必要重用了。
            if (lastBufferUsed + 256 > BUFFER_BUF_LEN) {
                lastReadBuffer = ByteBuf::alloc();
                lastBufferUsed = 0;
            }
            firstReadBuffer = lastReadBuffer;
        }

        buf->len = BUFFER_BUF_LEN - lastBufferUsed;
        buf->base = lastReadBuffer->buf + lastBufferUsed;
    }

    int RequestHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        assert(lastReadBuffer);
        lastBufferUsed += nread;
        readPkgLen += nread;

        while (readPkgLen && readPkgLen >= packageLen) {
            ServiceNamePtr serviceName = reinterpret_cast<ServiceNamePtr>(firstReadBuffer->buf + firstBufferOffset);
            uint32_t serviceLen = CONVERT_VAL_8(serviceName->len);

            if (!packageLen) {
                if (serviceLen > MAX_SERVICE_LEN) {
                    // 不合法的地址
                    LOG(WARNING) << "超过MAX_SERVICE_LEN 238:" << serviceLen;
                    ch->close();
                    return -1;
                }

                if (readPkgLen <= serviceLen + REQ_HEADER_LEN) {
                    break;
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

                    auto pHeader = serviceName->last<RequestId>();
                    packageLen = headLen + CONVERT_VAL_32(pHeader->bodyLen);
                    setResponseChannelId(pHeader);

                    int headerBeforeClientIdLen = serviceLen + offsetof(RequestId, clientId) + 1;
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
                            // 上一个buffer一点都没复制，这个buffer就复制8个字节
                            memcpy(firstReadBuffer->next->buf - firstBufferCopyLen,// 头
                                   tmpHead + headerBeforeClientIdLen,
                                   8);
                        } else {
                            memcpy(firstReadBuffer->next->buf,// 头
                                   tmpHead + headerBeforeClientIdLen,
                                   secondHalf - 5);//至少有5个字节不用复制
                        }

                    }
                } else {
                    auto pHeader = serviceName->last<RequestId>();
                    packageLen = headLen + pHeader->bodyLen;
                    setResponseChannelId(pHeader);
                }
            } else {

                if (tmpHead[0]) {
                    //当前发生了拆包,header 在tmpHead中
                    serviceName = reinterpret_cast<ServiceNamePtr>(tmpHead);
                }
                // 从第一个buffer到消息的最后一个字节的长度.
                uint32_t firstBufferToMsgEndLen = packageLen + firstBufferOffset;
                Buffer *msgLastBuffer = firstReadBuffer;
                while (firstBufferToMsgEndLen >= BUFFER_BUF_LEN) {
                    // 占满
                    firstBufferToMsgEndLen -= BUFFER_BUF_LEN;
                    msgLastBuffer = msgLastBuffer->next;
                }
                if (firstBufferToMsgEndLen &&
                    (readPkgLen > packageLen || firstBufferToMsgEndLen + 256 <= BUFFER_BUF_LEN)) {
                    // 这个消息没有把最后一个buffer占满， 已经读取到了下一个消息的一部分或者这个这个buffer还有超过256个字节可以用
                    ++msgLastBuffer->refCount;// 最后一个buffer和下一个消息会重用一个buffer，所以引用计数+1
                }

                auto req = serviceName->last<RequestId>();

                uint32_t reqId = req->requestId;
                uint32_t clientId = req->clientId;
                uint32_t serverId = req->serverId;

                // find service and send msg;
                ServiceKeeper *serviceKeeper = findOutCh(serviceName);
                int status = -1;
                if (serviceKeeper) {
                    auto serviceSize = serviceKeeper->count();
                    for (int i = 0; i < serviceSize; ++i) {
                        status = serviceKeeper->getChannel()->writeMsg(firstReadBuffer, firstBufferOffset, packageLen);
                        if (status != -1) {
                            break;
                        }
                    }
                }

                if (status < 0) {

                    if (status == -1) {
                        // 回收掉
                        ByteBuf::recycleWrited(firstReadBuffer, firstBufferOffset, packageLen);
                    }

                    Buffer *errBuffer = ByteBuf::alloc();
                    auto *responseId = reinterpret_cast<ResponseId *>(errBuffer->buf);
                    bzero(responseId, sizeof(ResponseId));
                    responseId->headerLen = CONVERT_VAL_8(sizeof(ResponseId));
                    responseId->requestId = reqId;
                    responseId->clientId = clientId;
                    responseId->serverId = serverId;
                    responseId->responseCode = status == -1 ? CONVERT_VAL_8(SKYNET_ERR_TRANSFER)
                                                            : CONVERT_VAL_8(SKYNET_ERR_NO_SERVICE);
                    if (ch->writeMsg(errBuffer, 0, sizeof(ResponseId)) == UV_EOF) {
                        ByteBuf::recycleSingle(errBuffer);
                    }
                }

                // 清理掉临时header
                tmpHead[0] = 0;
                readPkgLen -= packageLen;
                packageLen = 0;

                firstReadBuffer = msgLastBuffer;
                firstBufferOffset = firstBufferToMsgEndLen;
            }
        }

        return 0;
    }

    void RequestHandler::onClose(const uv_buf_t *buf) {
        if (firstReadBuffer) {
            ByteBuf::recycleWrited(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    void RequestHandler::onError(const uv_buf_t *buf) {
        if (firstReadBuffer) {
            ByteBuf::recycleWrited(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    ClientAppHandler::ClientAppHandler(const shared_ptr<Channel> &ch) : RequestHandler(ch), valid(false) {
    }

    ServiceKeeper *ClientAppHandler::findOutCh(ServiceNamePtr serviceName) {
        return Thread::local<Client>().getByService(serviceName);
    }

    void ClientAppHandler::setResponseChannelId(RequestId *header) {
        header->clientId = ch->channelId();
    }


    ServerReqHandler::ServerReqHandler(const shared_ptr<Channel> &ch) : RequestHandler(ch) {}

    ServiceKeeper *ServerReqHandler::findOutCh(ServiceNamePtr serviceName) {
        return nullptr;
    }

    void ServerReqHandler::setResponseChannelId(RequestId *header) {
        header->serverId = ch->channelId();
    }

}
