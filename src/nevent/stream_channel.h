//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_STREAM_CHANNEL_H
#define SKYNET_STREAM_CHANNEL_H

#include <glog/logging.h>
#include "channel.h"
#include "channel_handler.h"
#include <vector>
#include <queue>


namespace sn {

    struct WriteEvent {
        uint32_t dataLen;
        uint32_t bufferOffset;
        IoBuf *buffer;
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
                ByteBuf::recycleLen(evt.buffer, evt.bufferOffset, evt.dataLen);
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

        static void recycleWrited(Buffer *buffer, uint32_t firstOffset, uint32_t writed,
                                  uint32_t *unwritedOffset, Buffer **unwritedBuffer) {
            int rLen = BUFFER_BUF_LEN - firstOffset;
            Buffer *tmp = buffer;
            Buffer *next = tmp->next;
            while (writed > rLen) {
                assert(next);
                if (--tmp->refCount) {
                    ByteBuf::recycleSingle(tmp);
                }
                tmp = next;
                rLen += BUFFER_BUF_LEN;
                next = tmp->next;
            }

            // 两个相等还有最后一个没回收
            if (writed == rLen) {
                if (unwritedOffset) {
                    *unwritedOffset = 0;
                }
                if (unwritedBuffer) {
                    *unwritedBuffer = next;
                }
                if (--tmp->refCount) {
                    ByteBuf::recycleSingle(tmp);
                }
            } else {
                // 如果发现最后一个没有写完，是不会回收最后一个的
                if (unwritedOffset) {
                    *unwritedOffset = BUFFER_BUF_LEN - (rLen - writed);
                }
                if (unwritedBuffer) {
                    *unwritedBuffer = tmp;
                }
            }

        }

        /**
         * 返回
         * @param buffer 数据buffer链表头
         * @param firstOffset 数据链表头第一个buffer偏移量
         * @param len 数据长度
         * @return >= 0 写成功、 -1 出错但是buffer还没被污染可自行处理， <-1 buffer已经被回收掉了
         */
        int writeMsg(IoBuf &buf) override {
            if (closed) {
                // recycleWrited(buffer, firstOffset, len);
                return -1;
            }
            int len = buf.getSize();

            int writed = 0;

            // tryWrite 不能写过多数据。因为缓冲区写满了就无法再写了.208K
            if (writingQue.empty() && buf.getSize() < 212992) {

                auto blockSize = buf.dataBlockSize();

                if (blockSize == 1) {
                    uv_buf_t uvBuf;
                    buf.firstDataPtr(uvBuf);
                    writed = uv_try_write((uv_stream_t *) &handle, &uvBuf, 1);
                } else {
                    uv_buf_t uvBuf[blockSize];
                    writed = uv_try_write((uv_stream_t *) &handle, uvBuf, blockSize);
                }

                if (writed == UV_EAGAIN) {
                    // 到这里的概率很小.
                    writed = 0;
                } else if (writed < 0) {
                    LOG(ERROR) << " Write error on " << channelId() << ":" << uv_err_name(writed);
                    close();
                    // 返回eof调用者还可以找另一个channel重发或自行处理
                    return -1;
                } else if (len == writed) {
                    return writed;
                } else {
                    buf.clear(writed);
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
                        ByteBuf::recycleLen(buffer, firstOffset, len);
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
                uv_close((uv_handle_t *) &handle, ChannelHandler::onChannelClosed);
            }

        }

    private:
        void writeNextWaitingEvent() {
            auto size = waitingWrite.size();
            if (!size) {
                return;
            }

            std::vector<uv_buf_t> wes;
            wes.reserve(size);

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
                status = uv_write(&wrHandle, (uv_stream_t *) &handle, &buf, 1, onWritingCompleted);
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

}


#endif //SKYNET_STREAM_CHANNEL_H
