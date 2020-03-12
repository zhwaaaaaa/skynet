//
// Created by dear on 2020/2/10.
//

#include <event/server.h>
#include "shake_hands_handler.h"
#include "ResponseHandler.h"

namespace sn {

    ShakeHandsHandler::ShakeHandsHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch), readBytes(0), firstBuffer(nullptr), lastBuffer(nullptr),
              lastBufferOffset(0), serviceSize(0), shakePkgLen(0) {
    }

    void ShakeHandsHandler::onMemoryRequired(size_t suggested_size, uv_buf_t *buf) {
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

    int ShakeHandsHandler::onMessage(const uv_buf_t *buf, ssize_t nread) {
        readBytes += nread;
        lastBufferOffset += nread;

        if (readBytes < shakePkgLen || (!shakePkgLen && readBytes < 6)) {
            // 没有读满一个包，或者包长度没解析出来并且现在还不够解析
            return 0;
        }
        ShakeHandsHeader *header = reinterpret_cast<ShakeHandsHeader *>(firstBuffer->buf);
        if (!shakePkgLen) {
            shakePkgLen = CONVERT_VAL_32(header->len) + 4;
            if (SERVICE_TYPE_CLIENT != header->shakeType
                && SERVICE_TYPE_SERVER != header->shakeType) {
                ch->close();
                return -1;
            }
            serviceSize = CONVERT_VAL_16(header->serviceSize);
            if (serviceSize == 0) {
                // 握手成功之前，已经发数据过来了，关闭
                ch->close();
                return -1;
            }
        }
        if (readBytes < shakePkgLen) {
            return 0;
        }
        bool success = false;
        if (readBytes == shakePkgLen) {
            if (!doShakeHands(firstBuffer, readBytes, serviceSize)) {
                success = true;
            }
        }
        // 握手成功之前，已经发数据过来了，也算失败。关闭

        if (success) {
            // 握手成功这个handler不需要了，自杀
            delete this;
            return 0;
        } else {
            ch->close();
            return -1;
        }
    }

    void ShakeHandsHandler::onClose(const uv_buf_t *buf) {
        if (firstBuffer) {
            ByteBuf::recycleLen(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
        ch->close();
    }

    void ShakeHandsHandler::onError(const uv_buf_t *buf) {
        if (firstBuffer) {
            ByteBuf::recycleLen(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
        ch->close();
    }

    ShakeHandsHandler::~ShakeHandsHandler() {
        if (firstBuffer) {
            ByteBuf::recycleLen(firstBuffer, 0, readBytes);
            firstBuffer = nullptr;
        }
    }

    ClientShakeHandsHandler::ClientShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ClientShakeHandsHandler::doShakeHands(Buffer *firstBuffer, uint32_t pkgLen, int serviceSize) {
        vector<string> servs;
        ServiceNamePtr serv = nullptr;
        Buffer *tmp = firstBuffer;
        int useTmpOffset = 7;// 前面有7个字节
        int decodeBytes = 7;
        int decodeServices = 0;
        do {
            if (decodeBytes >= pkgLen) {
                // 没有解析完，字节数已经够了？
                return -1;
            }
            if (useTmpOffset != -1) {
                serv = reinterpret_cast<ServiceNamePtr>(tmp->buf + useTmpOffset);
                useTmpOffset = -1;
            } else {
                serv = serv->last();
            }
            if (serv->len > 238) {
                // serviceName的长度一定<=238
                return -1;
            }
            auto bufferLef = decodeBytes % BUFFER_BUF_LEN;
            if (bufferLef + serv->len + 1 > BUFFER_BUF_LEN) {
                // 当前service跨越两个Buffer。buffer剩下的长度-1 就是前半个buffer复制的数量
                uint32_t topHalf = BUFFER_BUF_LEN - 1 - bufferLef;
                string t(serv->buf, topHalf);
                tmp = tmp->next;
                auto secondHalf = serv->len - topHalf;
                t.append(tmp->buf, secondHalf);
                servs.emplace_back(std::move(t));
                useTmpOffset = secondHalf;
            } else if (bufferLef + serv->len + 1 == BUFFER_BUF_LEN) {
                // 刚好把这个buffer用完
                servs.emplace_back(serv->buf, serv->len);
                tmp = tmp->next;
                useTmpOffset = 0;
            } else {
                servs.emplace_back(serv->buf, serv->len);
            }
            decodeBytes += serv->len + 1;

        } while (++decodeServices < serviceSize);
        if (decodeBytes != pkgLen) {
            return -1;
        }

        Buffer *retFirstBuf = tmp = ByteBuf::alloc();

        // [4:len][2:serviceSize][1:singleSize][0-238][1:providerSize]:
        // 因为每个service多了一个providerSize.所以最终字节数算出来是.
        uint32_t retPkgBytes = pkgLen - 7 + serviceSize + 2 + 4;
        int require = retPkgBytes - BUFFER_BUF_LEN;
        while (require > 0) {
            tmp->next = ByteBuf::alloc();
            tmp = tmp->next;
            require -= BUFFER_BUF_LEN;
        }

        tmp = retFirstBuf;
        WRITELE_VAL_32(tmp->buf, retPkgBytes - 4);
        WRITELE_VAL_16(tmp->buf + 4, serviceSize);
        uint32_t writed = 6;
        Client &client = Thread::local<Client>();
        for (const string &s:servs) {
            auto i = client.enableService(ch->channelId(), s);
            auto currentBufferUsed = writed % BUFFER_BUF_LEN;
            if (currentBufferUsed) {
                if (currentBufferUsed + s.size() + 2 > BUFFER_BUF_LEN) {
                    // 当前buffer剩下的已经不足 以写了
                    char t[s.size() + 2];
                    WRITELE_VAL_8(t, s.size());
                    memcpy(t + 1, s.data(), s.size());
                    WRITELE_VAL_8(t + s.size() + 1, i);
                    uint32_t topHalf = BUFFER_BUF_LEN - currentBufferUsed;
                    memcpy(tmp->buf + currentBufferUsed, t, topHalf);
                    tmp = tmp->next;
                    memcpy(tmp->buf, t + topHalf, s.size() + 2 - topHalf);
                } else {
                    tmp->buf[currentBufferUsed] = s.size();
                    memcpy(tmp->buf + currentBufferUsed + 1, s.data(), s.size());
                    tmp->buf[currentBufferUsed + s.size() + 1] = (uint8_t) i;
                }
            } else {
                tmp = tmp->next;
                tmp->buf[0] = s.size();
                memcpy(tmp->buf + 1, s.data(), s.size());
                tmp->buf[s.size() + 1] = (uint8_t) i;
            }
            writed += s.size() + 2;
        }

        assert(writed == retPkgBytes);
        int r;
        if ((r = ch->writeMsg(retFirstBuf, 0, retPkgBytes)) < 0) {
            if (r == -1) {
                ByteBuf::recycleLen(retFirstBuf, 0, retPkgBytes);
            }
            return -1;
        }

        LOG(INFO) << "Client握手成功,serviceSize:" << serviceSize;
        auto handler = new ClientAppHandler(ch, std::move(servs));
        auto pHandler = ch->replaceHandler(handler);
        assert(pHandler == this);
        return 0;
    }


    ServerShakeHandsHandler::ServerShakeHandsHandler(const shared_ptr<Channel> &ch) : ShakeHandsHandler(ch) {
    }

    int ServerShakeHandsHandler::doShakeHands(Buffer *firstBuf, uint32_t pkgLen, int serviceSize) {
        vector<ServiceDesc> sds;
        sds.reserve(serviceSize);

        int contentLen = (int) pkgLen - 7;

        char *buf = static_cast<char *>(malloc(contentLen));
        if (!buf) {
            return -1;
        }
        auto delFree = [](char *b) {
            free(b);
        };
        unique_ptr<char, decltype(delFree)> guard(buf, delFree);

        int leftLen = contentLen;
        Buffer *tmp = firstBuf;
        int cpLen = min((int) BUFFER_BUF_LEN - 7, leftLen);
        memcpy(buf + contentLen - leftLen, tmp->buf + 7, cpLen);
        leftLen -= cpLen;
        while (leftLen > 0) {
            tmp = tmp->next;
            cpLen = min((int) BUFFER_BUF_LEN, leftLen);
            memcpy(buf + contentLen - leftLen, tmp->buf, cpLen);
            leftLen -= cpLen;
        }
        assert(leftLen == 0);

        vector<string> servs;
        servs.reserve(serviceSize);

        int decodeBytes = 0;
        while (decodeBytes < contentLen) {
            ServiceDesc desc{};
            desc.name = reinterpret_cast<ServiceNamePtr>(buf + decodeBytes);
            servs.emplace_back(desc.name->buf, desc.name->len);
            decodeBytes += desc.name->len + 1;
            if (decodeBytes >= contentLen) {
                return -1;
            }
            desc.param = reinterpret_cast<BodyDescPtr>(buf + decodeBytes);
            decodeBytes += desc.param->len + 4;
            if (decodeBytes >= contentLen) {
                return -1;
            }
            desc.result = reinterpret_cast<BodyDescPtr>(buf + decodeBytes);
            decodeBytes += desc.result->len + 4;
            sds.push_back(desc);
        }
        if (decodeBytes != contentLen) {
            return -1;
        }
        if (sds.size() != serviceSize) {
            return -1;
        }

        Thread::local<Server>().addServerAppChannel(ch, sds);

        Buffer *retFirstBuf = tmp = ByteBuf::alloc();

        //计算返回字节数
        uint32_t retPkgBytes = 6;
        for (const auto &s:servs) {
            retPkgBytes += s.size() + 2;
        }

        int require = retPkgBytes - BUFFER_BUF_LEN;
        while (require > 0) {
            tmp->next = ByteBuf::alloc();
            tmp = tmp->next;
            require -= BUFFER_BUF_LEN;
        }

        tmp = retFirstBuf;
        WRITELE_VAL_32(tmp->buf, retPkgBytes - 4);
        WRITELE_VAL_16(tmp->buf + 4, serviceSize);
        uint32_t writed = 6;
        for (const string &s:servs) {
            auto currentBufferUsed = writed % BUFFER_BUF_LEN;
            if (currentBufferUsed) {
                if (currentBufferUsed + s.size() + 2 > BUFFER_BUF_LEN) {
                    // 当前buffer剩下的已经不足 以写了
                    char t[s.size() + 2];
                    WRITELE_VAL_8(t, s.size());
                    memcpy(t + 1, s.data(), s.size());
                    WRITELE_VAL_8(s.size() + 1, 1);
                    uint32_t topHalf = BUFFER_BUF_LEN - currentBufferUsed;
                    memcpy(tmp->buf + currentBufferUsed, t, topHalf);
                    tmp = tmp->next;
                    memcpy(tmp->buf, t + topHalf, s.size() + 2 - topHalf);
                } else {
                    tmp->buf[currentBufferUsed] = s.size();
                    memcpy(tmp->buf + currentBufferUsed + 1, s.data(), s.size());
                    tmp->buf[currentBufferUsed + s.size() + 1] = (uint8_t) 1;
                }
            } else {
                tmp = tmp->next;
                tmp->buf[0] = s.size();
                memcpy(tmp->buf + 1, s.data(), s.size());
                tmp->buf[s.size() + 1] = (uint8_t) 1;
            }
            writed += s.size() + 2;
        }

        assert(writed == retPkgBytes);
        int r;
        if ((r = ch->writeMsg(retFirstBuf, 0, retPkgBytes)) < 0) {
            if (r == -1) {
                ByteBuf::recycleLen(retFirstBuf, 0, retPkgBytes);
            }
            return -1;
        }
        LOG(INFO) << "Server握手成功,serviceSize:" << serviceSize;

        auto handler = new ServerAppHandler(ch, std::move(servs));
        auto pHandler = ch->replaceHandler(handler);
        assert(pHandler == this);
        return 0;
    }
}
