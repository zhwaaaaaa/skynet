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


    using WriteEventQueue = std::deque<IoBuf>;

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
            CHECK(!writingQue.empty()) << "writing size 0?";
            // 不管对错，先把正在写的回收掉
            writingQue.clear();
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
        int writeMsg(IoBuf &buf) override {
            if (closed) {
                // recycleWrited(buffer, firstOffset, len);
                return -1;
            }
            int len = buf.getSize();
            CHECK(len > 0) << "no data write?";

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
                    buf.dataPtr(uvBuf, blockSize);
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
                    waitingWrite.emplace_back(std::move(buf));
                    return writed;
                }

                int status = writeAsync(buf);
                if (status) {
                    if (!writed) {
                        // 如果之前没有写入操作,返回一个错误，这个buffer还可以重新找一个管道写出去。
                        return -1;
                    } else {
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

            for (const IoBuf &evt: waitingWrite) {
                uint32_t blockSize = evt.dataBlockSize();
                if (blockSize == 1) {
                    uv_buf_t uvBuf;
                    evt.firstDataPtr(uvBuf);
                    wes.push_back(uvBuf);
                } else {
                    uv_buf_t uvBuf[blockSize];
                    evt.dataPtr(uvBuf, blockSize);
                    for (int i = 0; i < blockSize; ++i) {
                        wes.push_back(uvBuf[i]);
                    }
                }
            }

            int status;
            if (!(status = uv_write(&wrHandle, (uv_stream_t *) &handle, wes.data(), wes.size(), onWritingCompleted))) {
                writingQue = std::move(waitingWrite);
            } else {
                LOG(ERROR) << "Error Write:" << uv_err_name(status);
                close();
            }
        }

        int writeAsync(IoBuf &buf) {

            uint32_t blockSize = buf.dataBlockSize();
            CHECK(blockSize > 0);
            int status;
            if (blockSize == 1) {
                uv_buf_t uvBuf;
                buf.firstDataPtr(uvBuf);
                status = uv_write(&wrHandle, (uv_stream_t *) &handle, &uvBuf, 1, onWritingCompleted);
            } else {
                uv_buf_t uvBuf[blockSize];
                buf.dataPtr(uvBuf, blockSize);
                status = uv_write(&wrHandle, (uv_stream_t *) &handle, uvBuf, blockSize, onWritingCompleted);
            }

            if (!status) { // status 为0 代表写成功
                writingQue.push_back(std::move(buf));
            }

            return status;
        }
    };

}


#endif //SKYNET_STREAM_CHANNEL_H
