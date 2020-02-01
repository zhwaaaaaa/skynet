//
// Created by dear on 19-12-22.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>
#include "conn_channel.h"
#include <glog/logging.h>

namespace sn {
    using namespace google;

    int ConnChannel::doRead() {
        size_t nbytes;
        ssize_t readed;
        do {
            nbytes = buf.canWriteSize();
            readed = read(fd, buf.writePtr(), nbytes);
            if (readed == -1) {
                if (errno != EAGAIN) {
                    LOG(WARNING) << "ERROR: " << strerror(errno);
                    return -1;
                } else {
                    break;
                }
            }
            if (readed == 0) {
                LOG(WARNING) << "CLOSED: ";
                return -1;
            }

            buf.addWrited(readed);
//            if (decodeStatus == READING_HEAD) {
            if (tryDecodeBuf() != 0) {
                return -1;
            }
        } while (readed < nbytes);
        return 0;
    }


    int ConnChannel::decodeHead(Segment<uint8_t> *head, Header *header) {
        Segment<unsigned char> *const serv = head->sub();
        Segment<uint8_t> *const methodName = head->sub(serv->len + sizeof(uint8_t));

        uint headLen = static_cast<uint>(head->len);
        if (headLen != static_cast<uint>(serv->len) + methodName->len + 18) {
            LOG(WARNING) << "Service Name Len " << serv->len << ",Method Name Len " << methodName->len;
            return -1;
        }

        /**
         * |1|1|service|1|method|4reqId|4clientId|4serverId|4bodyLen|body|
         */
        header->servName = serv;
        header->methodName = methodName;
        header->reqId = reinterpret_cast<uint32_t *>(head->buf + headLen - 16);
        header->clientId = reinterpret_cast<uint32_t *>(head->buf + headLen - 12);
        header->serverId = reinterpret_cast<uint32_t *>(head->buf + headLen - 8);
        header->bodyLen = *reinterpret_cast<uint32_t *>(head->buf + headLen - 4);
        return 0;
    }

    int ConnChannel::tryDecodeBuf() {
        size_t size = buf.canReadSize();
        uint8_t len = buf.readUint<uint8_t>();
        if (size >= len) {
            // header读完成
            Header header;
            Segment<uint8_t> *const head = buf.segment<uint8_t>();
            int r = this->decodeHead(head, &header);
            if (r != 0) {
                LOG(WARNING) << "ERROR DECODE: ";
                return -1;
            }
            LOG(INFO) << "Service" << *header.servName << *header.methodName
                      << "[reqId:" << *header.clientId << "][bodyLen:" << header.bodyLen << "]";
            // TODO transfer to server
        }
        return 0;
    }

    static thread_local WriteEvt *cacheWriteEvt;

    static WriteEvt *createWriteEvt(writeFunc func, void *param) {
        WriteEvt *evt = cacheWriteEvt;
        WriteEvt *r;
        if (evt) {
            r = evt;
            cacheWriteEvt = r->nextEvt;
        } else {
            r = static_cast<WriteEvt *>(malloc(sizeof(WriteEvt)));
        }
        r->func = func;
        r->param = param;
        r->nextEvt = nullptr;
        return r;
    }

    static WriteEvt *deleteWriteEvtGetNext(WriteEvt *evt) {
        WriteEvt *r = evt->nextEvt;
        evt->nextEvt = cacheWriteEvt;
        cacheWriteEvt = evt;
        return r;
    }

    int ConnChannel::doWrite() {
        WriteEvt *evt;
        int ret = 0;
        while ((evt = headEvt)) {
            Action action = evt->func(CAN_WRITE, this, evt->param);
            switch (action) {
                case ERROR:
                    headEvt = deleteWriteEvtGetNext(evt);
                    ret = -1;
                    goto out;
                case SUCCESS:
                    headEvt = deleteWriteEvtGetNext(evt);
                    continue;
                case NEXT:
                    return 0;
            }
        }
        out:
        if (!evt) {
            tailEvt = nullptr;
            // hav nothing to write
            clearWrite();
        }
        return ret;
    }


    int ConnChannel::AddWriteEvt(writeFunc func, void *param) {
        if (!tailEvt) {
            // 以前还没有要写的东西，所以先尝试第一次写。
            Action action = func(TRY_WRITE, this, param);
            switch (action) {
                case SUCCESS:
                    // 第一次写就把东西写完了。不需要添加到写队列中了
                    return 0;
                case ERROR:
                    return -1;
                case NEXT:
                    headEvt = tailEvt = createWriteEvt(func, param);
                    // having something to write
            }
        } else {
            tailEvt->nextEvt = createWriteEvt(func, param);
            tailEvt = tailEvt->nextEvt;
        }
        markWrite();
        return 0;
    }

    ConnChannel::ConnChannel(int fd) :
            Channel(), buf(16384), decodeStatus(READING_HEAD), headEvt(nullptr), tailEvt(nullptr) {
        this->fd = fd;
    }

}
