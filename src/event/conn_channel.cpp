//
// Created by dear on 19-12-22.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <iostream>
#include "conn_channel.h"
#include <glog/logging.h>
// 256-1(头长度)-1(serviceName长度)-1(methondName长度)-4(reqId) - 4(client填充id) -4(server填充id)
#define HEAD_METHOD_LEN 237

namespace sn {
    using namespace google;

    static int setTcpNoDelay(int fd, int val) {
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
            return -1;
        }
        return 0;
    }

    static int setTcpKeepAlive(int fd) {
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
            return -1;
        }
        return 0;
    }

    int ConnChannel::Init() {
        setTcpNoDelay(fd, 1);
        setTcpKeepAlive(fd);
        return 0;
    }

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
            if (tryDecodeBuf() != 0) {
                return -1;
            }
        } while (readed < nbytes);
        return 0;
    }


    int ConnChannel::decodeHead(Segment<uint8_t> *head, Header *header) {
        Segment<unsigned char> *const serv = head->sub();
        Segment<uint8_t> *const methodName = head->sub(serv->len + sizeof(uint8_t));
        const unsigned char len = serv->len + methodName->len;
        if (len > HEAD_METHOD_LEN) {
            LOG(WARNING) << "Service Name Len " << serv->len << ",Method Name Len " << methodName->len;
            return -1;
        }

        header->servName = serv;
        header->methodName = methodName;
        header->reqId = reinterpret_cast<uint32_t *>(head->buf + len + 2);
        header->clientId = reinterpret_cast<uint32_t *>(head->buf + len + 6);
        header->serverId = reinterpret_cast<uint32_t *>(head->buf + len + 10);
        header->bodyLen = *reinterpret_cast<uint32_t *>(head->buf + len + 14);
        return 0;
    }

    int ConnChannel::tryDecodeBuf() {
        size_t size = buf.canReadSize();
        uint8_t len = buf.readUnint<uint8_t>();
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


            buf.addReaded(len - 4);
            Segment<uint32_t> *const data = buf.segment<uint32_t>();
        }
        return 0;
    }

    void ConnChannel::doClose() {

    }

    int ConnChannel::doWrite() {
        return 0;
    }

}
