//
// Created by dear on 2020/2/6.
//

#include "transfer_handler.h"
#include <glog/logging.h>
#include <thread/thread.h>
#include <event/client.h>
#include <event/server.h>
#include "stream_channel.h"

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
            if (!lastReadBuffer) { // first will hit
                lastReadBuffer = firstReadBuffer = ByteBuf::alloc();
            }
            // 包已经解析完了。并且这个包也已经快满了，没有必要重用了。
            if (lastBufferUsed + 256 > BUFFER_BUF_LEN) {
                // 因为还没解析的字节数为0，所以，读指针和写指针相同
                lastReadBuffer = firstReadBuffer = ByteBuf::alloc();
                firstBufferOffset = lastBufferUsed = 0;
            }

        }

        buf->len = BUFFER_BUF_LEN - lastBufferUsed;
        buf->base = lastReadBuffer->buf + lastBufferUsed;
    }

    int RequestHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
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
                    uint32_t topHalf = BUFFER_BUF_LEN - firstBufferOffset;
                    memcpy(tmpHead, serviceName, topHalf);
                    uint32_t secondHalf = headLen - topHalf;
                    memcpy(tmpHead + topHalf, firstReadBuffer->next->buf, secondHalf);
                    serviceName = reinterpret_cast<ServiceNamePtr>(tmpHead);

                    auto pHeader = serviceName->last<RequestId>();
                    packageLen = headLen + CONVERT_VAL_32(pHeader->bodyLen);
                    setResponseChannelId(pHeader);

                    // clientId或者serverId的字段发生了修改。所以需要复制。
                    // 本来只有4个字节发生了修改。但是为了方便，把所有可能发生修改的8个字节都复制到buffer中
                    int headerBeforeClientIdLen = serviceLen + offsetof(RequestId, clientId) + 1;
                    // [prevPkg][headerBeforeClientId][clientId][serverId][1:bodyType][4:bodyLen]
                    //          |              topHalf               |      secondHalf
                    //    firstBufferOffset                    firstBufferEnd
                    int firstBufferCopyLen = topHalf - headerBeforeClientIdLen;
                    if (firstBufferCopyLen >= 8) {
                        // 复制的内容全部都在前半段。
                        memcpy(firstReadBuffer->buf + firstBufferOffset + headerBeforeClientIdLen,
                               tmpHead + headerBeforeClientIdLen,
                               8);//有可能超过8字节也没有关系。
                    } else if (firstBufferCopyLen > 0) {
                        //说明clientId,serverId 被拆开了，因为着两个字段有修改。所以需要复制两个buffer
                        //[prevPkg][headerBeforeClientId][clientId][serverId]
                        memcpy(firstReadBuffer->buf + firstBufferOffset + headerBeforeClientIdLen,
                               tmpHead + headerBeforeClientIdLen,
                               firstBufferCopyLen);//前半段复制一部分
                        memcpy(firstReadBuffer->next->buf, tmpHead + headerBeforeClientIdLen + firstBufferCopyLen,
                               8 - firstBufferCopyLen);// 后半段复制一部分，加起来8个字节
                    } else {
                        // 复制的内容全部都在后段。firstBufferCopyLen<=0的,复制的内容可能不是第二个buffer的第一个字节
                        memcpy(firstReadBuffer->next->buf - firstBufferCopyLen,
                               tmpHead + headerBeforeClientIdLen, 8);
                    }

                } else {
                    auto pHeader = serviceName->last<RequestId>();
                    packageLen = headLen + CONVERT_VAL_32(pHeader->bodyLen);
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
                    (firstBufferToMsgEndLen + 256 <= BUFFER_BUF_LEN || readPkgLen > packageLen)) {
                    // 这个消息没有把最后一个buffer占满， 已经读取到了下一个消息的一部分或者这个这个buffer还有超过256个字节可以用
                    ++msgLastBuffer->refCount;// 最后一个buffer和下一个消息会重用一个buffer，所以引用计数+1
                }

                auto req = serviceName->last<RequestId>();

                uint32_t reqId = req->requestId;
                uint32_t clientId = req->clientId;
                uint32_t serverId = req->serverId;


                // find service and send msg;
                ChannelGroup *serviceKeeper = findOutCh(serviceName);
                int status = -1;
                if (serviceKeeper) {
                    auto serviceSize = serviceKeeper->channelSize();
                    for (int i = 0; i < serviceSize; ++i) {
                        auto pChannel = serviceKeeper->nextChannel();
                        if (pChannel) {
                            status = pChannel->writeMsg(firstReadBuffer, firstBufferOffset, packageLen);
                            if (status != -1) {
                                break;
                            }
                        }
                    }
                }

                if (status < 0) {

                    if (status == -1) {
                        // 回收掉
                        ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, packageLen);
                    }

                    Buffer *errBuffer = ByteBuf::alloc();
                    auto *responseId = reinterpret_cast<ResponseId *>(errBuffer->buf);
                    responseId->headerLen = CONVERT_VAL_8(RESP_HEADER_LEN - 1);
                    responseId->requestId = reqId;
                    responseId->clientId = clientId;
                    responseId->serverId = serverId;
                    responseId->responseCode = status == -1 ? CONVERT_VAL_8(SKYNET_ERR_NO_SERVICE)
                                                            : CONVERT_VAL_8(SKYNET_ERR_TRANSFER);
                    responseId->bodyType = 0;
                    responseId->bodyLen = 0;
                    auto start = currentTimeUs();
                    if (ch->writeMsg(errBuffer, 0, sizeof(ResponseId)) == -1) {
                        ByteBuf::recycleSingle(errBuffer);
                    }
                    auto cost = currentTimeUs() - start;
                    if (cost > 2) {
                        LOG(INFO) << reqId << " writeMsg Time const " << cost;
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
            ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    void RequestHandler::onError(const uv_buf_t *buf) {
        if (firstReadBuffer) {
            ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    ClientAppHandler::ClientAppHandler(const shared_ptr<Channel> &ch, vector<string> &&serv)
            : RequestHandler(ch), requiredService(serv) {
        Thread::local<Client>().addResponseChannel(this->ch);
    }

    ChannelGroup *ClientAppHandler::findOutCh(ServiceNamePtr serviceName) {
        return Thread::local<Client>().getByService(serviceName);
    }

    void ClientAppHandler::setResponseChannelId(RequestId *header) {
        header->clientId = ch->channelId();
    }

    ClientAppHandler::~ClientAppHandler() {
        Thread::local<Client>().removeResponseChannel(ch->channelId());
        Client &client = Thread::local<Client>();
        for (const auto &s:requiredService) {
            client.disableService(ch->channelId(), s);
            LOG(INFO) << ch->channelId() << "with ClientAppHandler not require service " << s;
        }

    }


    ServerReqHandler::ServerReqHandler(const shared_ptr<Channel> &ch) : RequestHandler(ch) {
        Thread::local<Server>().addResponseChannel(this->ch);
    }

    ServerReqHandler::~ServerReqHandler() {
        Thread::local<Server>().removeResponseChannel(ch->channelId());
    }

    ChannelGroup *ServerReqHandler::findOutCh(ServiceNamePtr serviceName) {
        return Thread::local<Server>().getChannelByService(string_view(serviceName->buf, serviceName->len));
    }

    void ServerReqHandler::setResponseChannelId(RequestId *header) {
        header->serverId = ch->channelId();
    }


    ResponseHandler::ResponseHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), firstBufferOffset(0), lastBufferUsed(0), readPkgLen(0), packageLen(0),
              lastReadBuffer(nullptr), firstReadBuffer(nullptr) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    void ResponseHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {

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
            if (!lastReadBuffer) { // first will hit
                lastReadBuffer = firstReadBuffer = ByteBuf::alloc();
            }
            // 包已经解析完了。并且这个包也已经快满了，没有必要重用了。
            if (lastBufferUsed + 256 > BUFFER_BUF_LEN) {
                // 因为还没解析的字节数为0，所以，读指针和写指针相同
                lastReadBuffer = firstReadBuffer = ByteBuf::alloc();
                firstBufferOffset = lastBufferUsed = 0;
            }
        }

        buf->len = BUFFER_BUF_LEN - lastBufferUsed;
        buf->base = lastReadBuffer->buf + lastBufferUsed;
    }

    int ResponseHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {

        lastBufferUsed += nread;
        readPkgLen += nread;

        while (readPkgLen && readPkgLen >= packageLen) {
            ResponseId *responseId = reinterpret_cast<ResponseId *>(firstReadBuffer->buf + firstBufferOffset);
            if (RESP_HEADER_CONTENT_LEN != CONVERT_VAL_8(responseId->headerLen)) {
                LOG(WARNING) << "response head len !=" << RESP_HEADER_CONTENT_LEN;
                // resp header 固定18个字节
                ch->close();
                return -1;
            }

            if (!packageLen) {
                if (readPkgLen < RESP_HEADER_LEN) {
                    // 还不够19个字节
                    return 0;
                }

                if (firstBufferOffset + RESP_HEADER_LEN > BUFFER_BUF_LEN) {
                    // 发生了header拆包。
                    // 上半个buffer的字节数
                    uint32_t topHalf = BUFFER_BUF_LEN - firstBufferOffset;
                    memcpy(tmpHead, responseId, topHalf);
                    memcpy(tmpHead + topHalf, firstReadBuffer->next->buf, RESP_HEADER_LEN - topHalf);
                    responseId = reinterpret_cast<ResponseId *>(tmpHead);
                }

                packageLen = RESP_HEADER_LEN + CONVERT_VAL_32(responseId->bodyLen);
            } else {
                if (tmpHead[0]) {
                    responseId = reinterpret_cast<ResponseId *>(tmpHead);
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
                    (firstBufferToMsgEndLen + 256 <= BUFFER_BUF_LEN || readPkgLen > packageLen)) {
                    // 这个消息没有把最后一个buffer占满， 已经读取到了下一个消息的一部分或者这个这个buffer还有超过256个字节可以用
                    ++msgLastBuffer->refCount;// 最后一个buffer和下一个消息会重用一个buffer，所以引用计数+1
                }

                // find channel and send msg;
                Channel *outChannel = findTransferChannel(responseId);
                if (!outChannel || outChannel->writeMsg(firstReadBuffer, firstBufferOffset, packageLen) == -1) {
                    ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, packageLen);
                }

                readPkgLen -= packageLen;
                packageLen = 0;
                tmpHead[0] = 0;

                firstReadBuffer = msgLastBuffer;
                firstBufferOffset = firstBufferToMsgEndLen;
            }
        }

        return 0;
    }

    void ResponseHandler::onClose(const uv_buf_t *buf) {
        if (firstReadBuffer) {
            ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    void ResponseHandler::onError(const uv_buf_t *buf) {
        if (firstReadBuffer) {
            ByteBuf::recycleLen(firstReadBuffer, firstBufferOffset, readPkgLen);
            firstReadBuffer = nullptr;
            firstBufferOffset = 0;
        }
        ch->close();
    }

    ClientResponseHandler::ClientResponseHandler(const shared_ptr<Channel> &ch) : ResponseHandler(ch) {
        // client response handler负责转发server的的返回。
        // 但它也负责写ClientAppHandler转发的请求
        // 所以要注册自己的channel到Client上。以便于ClientAppHandler可以使用
        // 因为对于客户端来说，它需要连到外部的channel上去。所以它一定是TcpChannel
        TcpChannel<ClientResponseHandler> *channel = dynamic_cast<TcpChannel<ClientResponseHandler> *>(ch.get());
        auto point = channel->remoteAddr();
        LOG(INFO) << "ClientResponseHandler() 连到server：" << point;
        Thread::local<Client>().getServiceChannel(point)->resetChannel(this->ch);
    }

    ClientResponseHandler::~ClientResponseHandler() {
        // 当连接断掉了
        TcpChannel<ClientResponseHandler> *channel = dynamic_cast<TcpChannel<ClientResponseHandler> *>(ch.get());
        auto point = channel->remoteAddr();
        LOG(INFO) << "~ClientResponseHandler() 断开server：" << point;
        Thread::local<Client>().removeServiceChannel(point);
    }

    Channel *ClientResponseHandler::findTransferChannel(ResponseId *responseId) {
        return Thread::local<Client>().getResponseChannel(responseId->clientId);
    }


    ServerAppHandler::ServerAppHandler(const shared_ptr<Channel> &ch, vector<string> &&provideServs)
            : ResponseHandler(ch), provideServs(provideServs) {}

    ServerAppHandler::~ServerAppHandler() {
        Thread::local<Server>().removeServerAppChannel(ch, provideServs);
    }

    Channel *ServerAppHandler::findTransferChannel(ResponseId *responseId) {
        return Thread::local<Server>().getResponseChannel(responseId->serverId);
    }


}
