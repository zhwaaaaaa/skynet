//
// Created by dear on 20-1-29.
//

#include <glog/logging.h>
#include "shake_hands_channel.h"
#include "tcp_channel.h"
#include <memory>

namespace sn {

    ShakeHandsChannel::ShakeHandsChannel(int fd, ServiceRegistry *serviceRegistry)
            : ConnChannel(fd), serviceRegistry(serviceRegistry) {
        connType = -1;
    }


    int ShakeHandsChannel::tryDecodeBuf() {
        const uint readedSize = buf.canReadSize();
        if (packageLen == -1) {
            if (readedSize >= 8) {
                Segment<uint32_t> *const segment = buf.segment<uint32_t>();
                packageLen = static_cast<int>(SEGMENT_LEN_32(segment));
                uint32_t maskAndSize = SEGMENT_LEN_32(segment->sub());
                serviceSize = static_cast<int>(maskAndSize & 0xFFFFFF);
                connType = static_cast<int>(maskAndSize >> 24);

                resp = new Buffer(static_cast<const uint32_t>(packageLen + 4 + serviceSize));
                LOG(INFO) << "packageLen:" << packageLen << ",serviceSize:"
                          << serviceSize << ",connType:" << connType;
            } else {
                // 不足8个字节,下一次继续读
                return 0;
            }
        }

        if (packageLen + 4 < readedSize) {
            // 没有把包读完
            return 0;
        }

        if (packageLen + 4 == readedSize) {
            LOG(INFO) << "readPackage success, len=" << packageLen;
            clearRead();// 不再读了

            if (readServicesAndConnect() != 0) {
                delete resp;
                resp = nullptr;
                status = -1;
                return -1;
            } else {
                status = 1;
            }

            this->AddWriteEvt([](int s, ConnChannel *ch, void *param) {
                auto *shc = dynamic_cast<ShakeHandsChannel *>(ch);
                return shc->writeData(s, ch, param);
            }, nullptr);

            return 0;
        }
        return -1;
    }


    int ShakeHandsChannel::readServicesAndConnect() {
        // |4packageLen|4serviceSize |1status|1serviceNameLen|serviceName|...
        char *ptr = resp->writePtr();
        // 相比于发送来的packageLen。只是每个service多了一个字节的状态
        int respPackageLen = packageLen + serviceSize;
        WRITELE_VAL_32(ptr, respPackageLen);
        ptr += 4;
        WRITELE_VAL_32(ptr, serviceSize);
        ptr += 4;
        resp->addWrited(8);
        int i = 0;
        Segment<uint8_t> *s = nullptr;
        do {
            s = s ? s->sub(s->len) : buf.segment<uint8_t>(8);
            int connSize = serviceRegistry->addRequiredService(s);
            if (connSize < 0) {
                return -1;
            }
            ptr[0] = static_cast<char>(connSize);
            mempcpy(ptr + 1, s, s->len + sizeof(uint8_t));
            resp->addWrited(s->len + 2);
        } while (++i < serviceSize);

        return 0;
    }


    void ShakeHandsChannel::doClose() {
        if (status == 2) {
            const auto channel = new TcpChannel(fd);
            fd = -1;
            dispatcher->DelChannelEvent(this);
            dispatcher->AddChannelEvent(channel, EVENT_READABLE);
            dispatcher = nullptr;
        } else {
            ConnChannel::doClose();
        }
    }


    Action ShakeHandsChannel::writeData(int s, ConnChannel *ch, void *param) {
        if (s != WRITE_ERR) {
            int writed = doWriteToFd();
            if (writed > 0) {
                return NEXT;
            } else if (writed == 0) {
                status = 2;
                return SUCCESS;
            }
        }

        status = -1;
        return ERROR;
    }

    int ShakeHandsChannel::doWriteToFd() {
        uint32_t s = resp->canReadSize();
        ssize_t writed = write(fd, resp->readPtr(), s);
        if (writed == -1) {
            if (errno != EAGAIN) {
                return -1;
            }
        } else if (writed == 0) {
            // 连接被关闭了，返回error
            return -1;
        }

        if (writed < s) {
            return static_cast<int>(s - writed);
        }
        return 0;
    }


}
