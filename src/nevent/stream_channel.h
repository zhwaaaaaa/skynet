//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_STREAM_CHANNEL_H
#define SKYNET_STREAM_CHANNEL_H

#include <glog/logging.h>
#include <cassert>
#include "channel.h"
#include "io_error.h"
#include "channel_handler.h"
#include <vector>
#include <queue>


namespace sn {

    struct WriteEvent {
        uint32_t dataLen;
        uint32_t bufferOffset;
        Buffer *buffer;
    };


    using WriteEventQueue = std::deque<WriteEvent>;

    uint32_t generateChannelId();

    template<class Type>
    class WritableChannel : public Channel {

    private:
        const uint32_t id;
        WriteEventQueue writingQue;
        // 16个字节就不做链表了
        WriteEventQueue waitingWrite;
        bool closed;
    private:
        static void onWritingCompleted(uv_write_t *req, int status) {
            WritableChannel<Type> *thisWriteCh = static_cast< WritableChannel<Type> *>(req->data);
            auto &writingQue = thisWriteCh->writingQue;
            // 不管对错，先把正在写的回收掉
            while (!writingQue.empty()) {
                const auto &evt = writingQue.front();
                ByteBuf::recycleWrited(evt.buffer, evt.bufferOffset, evt.dataLen);
                writingQue.pop_front();
            }
            if (status) {
                thisWriteCh->close();
            } else {
                thisWriteCh->writeNextWaitingEvent();
            }

        }

    protected:
        Type handle;
        uv_write_t wrHandle;
    public:
        WritableChannel() : writingQue(), waitingWrite(), closed(false), id(generateChannelId()) {
            wrHandle.data = this;
        }

        uint32_t channelId() override {
            return id;
        }

        ChannelHandler *replaceHandler(ChannelHandler *handler) override {
            ChannelHandler *old = static_cast<ChannelHandler *>(handle.data);
            handle.data = handler;
            return old;
        }

        /**
         * 返回
         * @param buffer 数据buffer链表头
         * @param firstOffset 数据链表头第一个buffer偏移量
         * @param len 数据长度
         * @return >= 0 写成功、 -1 出错但是buffer还没被污染可自行处理， <-1 buffer已经被回收掉了
         */
        int writeMsg(Buffer *buffer, uint32_t firstOffset, uint32_t len) override {
            if (closed) {
                // recycleWrited(buffer, firstOffset, len);
                return -1;
            }

            int writed = 0;

            // tryWrite 不能写过多数据。因为缓冲区写满了就无法再写了.208K
            if (writingQue.empty() && len < 212992) {
                if (len <= BUFFER_BUF_LEN - firstOffset) {
                    uv_buf_t buf = {buffer->buf + firstOffset, len};
                    writed = uv_try_write((uv_stream_t *) &handle, &buf, 0);
                } else {
                    auto exceptFirst = len - (BUFFER_BUF_LEN - firstOffset);
                    auto tailLen = exceptFirst % BUFFER_BUF_LEN;
                    uint32_t bufSize = exceptFirst / BUFFER_BUF_LEN + (tailLen == 0 ? 1 : 2);

                    uv_buf_t buf[bufSize];

                    Buffer *tmp = buffer;
                    buf[0] = {tmp->buf + firstOffset, BUFFER_BUF_LEN - firstOffset};

                    for (int i = 1; i < bufSize - 1; ++i) {
                        tmp = tmp->next;
                        buf[i].len = BUFFER_BUF_LEN;
                        buf[i].base = buffer->buf;
                    }

                    tmp = tmp->next;
                    buf[bufSize - 1] = {tmp->buf, tailLen};
                    writed = uv_try_write((uv_stream_t *) &handle, buf, bufSize);
                }


                if (writed == UV_EAGAIN) {
                    // 到这里的概率很小.
                    writed = 0;
                } else if (writed < 0) {
                    LOG(ERROR) << " Write error on " << channelId() << ":" << uv_err_name(writed);
                    close();
                    // 返回eof调用者还可以找另一个channel重发或自行处理
                    return -1;
                } else {
                    ByteBuf::recycleWrited(buffer, firstOffset, writed, &firstOffset, &buffer);
                    len -= writed;
                }
            }

            if (len) {
                if (!writingQue.empty()) {
                    waitingWrite.push_back({len, firstOffset, buffer});
                    return writed;
                }

                int status = writeAsync(buffer, firstOffset, len);
                if (status) {
                    if (!writed) {
                        // 如果之前没有写入操作,返回一个错误，这个buffer还可以重新找一个管道写出去。
                        return -1;
                    } else {
                        ByteBuf::recycleWrited(buffer, firstOffset, len);
                        close();
                        return -2;
                    }

                }
            }

            return writed;
        }

        void close() override {
            if (!closed) {
                closed = true;

                while (!writingQue.empty()) {
                    const auto &evt = writingQue.front();
                    ByteBuf::recycleWrited(evt.buffer, evt.bufferOffset, evt.dataLen);
                    writingQue.pop_front();
                }

                while (!waitingWrite.empty()) {
                    const auto &evt = waitingWrite.front();
                    ByteBuf::recycleWrited(evt.buffer, evt.bufferOffset, evt.dataLen);
                    waitingWrite.pop_front();
                }

                uv_close((uv_handle_t *) &handle, ChannelHandler::onChannelClosed);
            }

        }

    private:
        void writeNextWaitingEvent() {
            auto size = waitingWrite.size();
            if (!size) {
                return;
            }

            std::vector<uv_buf_t> wes(size);

            for (const WriteEvent &evt: waitingWrite) {
                if (evt.dataLen <= BUFFER_BUF_LEN - evt.bufferOffset) {
                    wes.push_back({evt.buffer->buf + evt.bufferOffset, evt.dataLen});
                } else {
                    auto exceptFirst = evt.dataLen - (BUFFER_BUF_LEN - evt.bufferOffset);
                    auto tailLen = exceptFirst % BUFFER_BUF_LEN;
                    uint32_t bufSize = exceptFirst / BUFFER_BUF_LEN + (tailLen == 0 ? 1 : 2);

                    Buffer *tmp = evt.buffer;
                    wes.push_back({tmp->buf + evt.bufferOffset, BUFFER_BUF_LEN - evt.bufferOffset});

                    for (int i = 1; i < bufSize - 1; ++i) {
                        tmp = tmp->next;
                        wes.push_back({evt.buffer->buf, BUFFER_BUF_LEN});
                    }

                    tmp = tmp->next;
                    wes.push_back({tmp->buf, tailLen});
                }
            }

            int status;
            if (!(status = uv_write(&wrHandle, (uv_stream_t *) &handle, wes.data(), wes.size(), onWritingCompleted))) {
                while (!waitingWrite.empty()) {
                    writingQue.push_back(waitingWrite.front());
                    waitingWrite.pop_front();
                }
            } else {
                LOG(ERROR) << "Error Write:" << uv_err_name(status);
                close();
            }
        }

        int writeAsync(Buffer *buffer, uint32_t firstOffset, uint32_t len) {

            int status;
            if (len <= BUFFER_BUF_LEN - firstOffset) {
                uv_buf_t buf = {buffer->buf + firstOffset, len};
                status = uv_write(&wrHandle, (uv_stream_t *) &handle, &buf, 0, onWritingCompleted);
            } else {
                auto exceptFirst = len - (BUFFER_BUF_LEN - firstOffset);
                auto tailLen = exceptFirst % BUFFER_BUF_LEN;
                uint32_t bufSize = exceptFirst / BUFFER_BUF_LEN + (tailLen == 0 ? 1 : 2);

                uv_buf_t buf[bufSize];

                Buffer *tmp = buffer;
                buf[0] = {tmp->buf + firstOffset, BUFFER_BUF_LEN - firstOffset};

                for (int i = 1; i < bufSize - 1; ++i) {
                    tmp = tmp->next;
                    buf[i].len = BUFFER_BUF_LEN;
                    buf[i].base = buffer->buf;
                }

                tmp = tmp->next;
                buf[bufSize - 1] = {tmp->buf, tailLen};
                status = uv_write(&wrHandle, (uv_stream_t *) &handle, buf, bufSize, onWritingCompleted);
            }

            if (!status) {
                writingQue.push_back({len, firstOffset, buffer});
            }

            return status;
        }
    };

    template<class Handler>
    class TcpChannel : public WritableChannel<uv_tcp_t> {
    private:
        EndPoint raddr;

        static void onConnected(uv_connect_t *req, int status) {
            if (!status) {
                free(req);
                return;
            }
            TcpChannel<Handler> *ch = static_cast<TcpChannel<Handler> *>(req->data);
            auto pHandler = new Handler(shared_ptr<TcpChannel<Handler>>(ch));
            uv_stream_t *stream = req->handle;
            stream->data = pHandler;
            uv_read_start(stream, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
        }

    public:

        ~TcpChannel() override {
            LOG(INFO) << "~TcpChannel()";
        }

        TcpChannel() {
            LOG(INFO) << "TcpChannel()";
        }

        EndPoint remoteAddr() {
            return raddr;
        }

        void addToLoop(uv_loop_t *loop) override {
            uv_tcp_init(loop, (uv_tcp_t *) &handle);
        }

        void connectTo(EndPoint endPoint) {
            raddr = endPoint;

            if (!handle.loop) {
                // lib_uv 每一个结构必须有一个loop
                addToLoop(uv_default_loop());
            }

            uv_connect_t *conType = static_cast<uv_connect_t *>(malloc(sizeof(uv_connect_t)));
            conType->data = this;
            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr = endPoint.ip;
            serv_addr.sin_port = htons(static_cast<uint16_t>(endPoint.port));
            int r;

            struct sockaddr_in client_addr;
            if ((r = uv_ip4_addr("0.0.0.0", 0, &client_addr))) {
                throw IoError(r, "Init local addr");
            }
            if ((r = uv_tcp_bind(&handle, (sockaddr *) &client_addr, 0))) {
                throw IoError(r, "Bind");
            }

            if ((r = uv_tcp_connect(conType, &handle, (sockaddr *) &serv_addr, onConnected))) {
                throw IoError(r, "Connection");
            }
            uv_tcp_keepalive(&handle, 1, 3600);
            uv_tcp_nodelay(&handle, 1);
        }

        void acceptFrom(uv_stream_t *server) {
            addToLoop(server->loop);
            int r;
            uv_stream_t *client = (uv_stream_t *) &handle;
            if ((r = uv_accept(server, client))) {
                throw IoError(r, "acceptFrom");
            }

            sockaddr_storage storage = {0};
            int len = sizeof(storage);

            if ((r = uv_tcp_getpeername(&handle, (sockaddr *) &storage, &len))) {
                throw IoError(r, "acceptFrom");
            }

            assert(storage.ss_family == AF_INET);
            sockaddr_in *in = reinterpret_cast<sockaddr_in *>(&storage);
            raddr = EndPoint(*in);
            LOG(INFO) << "Accept connection from " << raddr;
            handle.data = new Handler(shared_ptr<TcpChannel<Handler>>(this));
            uv_tcp_keepalive(&handle, 1, 3600);
            uv_tcp_nodelay(&handle, 1);
            uv_read_start(client, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
        };

    };

}


#endif //SKYNET_STREAM_CHANNEL_H
