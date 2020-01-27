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

    void ConnChannel::OnEvent(int mask) {
        if (mask & EVENT_CLOSE) {
            //
            LOG(INFO) << "closed: " << raddr;
            return;
        }

        if (mask & EVENT_READABLE) {
            size_t nbytes;
            ssize_t readed;
            do {
                nbytes = buf.canWriteSize();
                readed = read(fd, buf.writePtr(), nbytes);
                if (readed == -1) {
                    if (errno != EAGAIN) {
                        LOG(WARNING) << "ERROR: " << strerror(errno);
                        return;
                    }
                }
                if (readed == 0) {
                    LOG(WARNING) << "CLOSED: ";
                    return;
                }

                buf.addWrited(readed);
                size_t size = buf.canReadSize();
                uint8_t len = buf.readUnint<uint8_t>();
                if (size >= len) {
                    // header读完成
                    int r = this->decodeHead(buf.segment<uint8_t>());
                    if (r == -1) {
                        LOG(WARNING) << "ERROR DECODE: ";
                        return;
                    }

                    buf.addReaded(len - 4);
                    Segment<uint32_t> *const data = buf.segment<uint32_t>();

                }
            } while (readed < nbytes);


            if (decodeStatus == READING_HEAD) {

            }

        }


    }


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
        MarkNonBlock();
        setTcpNoDelay(fd, 1);
        setTcpKeepAlive(fd);
        return 0;
    }

    int ConnChannel::decodeHead(Segment<uint8_t> *head) {
        Segment<uint8_t> *const servName = head->sub();
        Segment<uint8_t> *const methodName = head->sub(servName->len + sizeof(uint8_t));
        return 0;
    }

}
