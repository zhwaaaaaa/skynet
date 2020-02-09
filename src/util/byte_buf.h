//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_BYTEBUF_H
#define SKYNET_BYTEBUF_H

#include <cstddef>
#include <uv-unix.h>
#include <cassert>
#include "buffer.h"

namespace sn {
    struct BufferPool {
        uint32_t size;
        Buffer *head;
        Buffer *tail;

        BufferPool() : size(0), head(nullptr), tail(nullptr) {}

        ~BufferPool() {
            Buffer *h = head;
            while (h) {
                Buffer *tmp = h;
                h = tmp->next;
                free(tmp);
            }
        }
    };


    class ByteBuf {
    private:
        static thread_local BufferPool pool;
    public:
        static Buffer *alloc() {
            Buffer *buffer;
            if (pool.size) {
                buffer = pool.head;
                pool.head = buffer->next;
                pool.size--;
            } else {
                buffer = static_cast<Buffer *>(malloc(BUFFER_SIZE));
            }
            buffer->next = nullptr;
            buffer->refCount = 1;
            buffer->next = nullptr;
            return buffer;
        }

        static void recycleWrited(Buffer *buffer, uint32_t firstOffset, uint32_t writed,
                                  uint32_t *unwritedOffset = nullptr, Buffer **unwritedBuffer = nullptr) {
            int writeLen = BUFFER_BUF_LEN - firstOffset;
            Buffer *tmp = buffer;
            Buffer *next = tmp->next;
            while (writed > writeLen) {
                assert(next);
                if (--tmp->refCount) {
                    ByteBuf::recycleSingle(tmp);
                }
                tmp = next;
                writeLen += BUFFER_BUF_LEN;
                next = tmp->next;
            }

            // 两个相等还有最后一个没回收
            if (writed == writeLen) {
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
                if (unwritedOffset) {
                    *unwritedOffset = BUFFER_BUF_LEN - (writeLen - writed);
                }
                if (unwritedBuffer) {
                    *unwritedBuffer = tmp;
                }
            }

        }

        static void recycle(uv_buf_t *buf) {
            Buffer *buffer = BUF_TO_BUFFER(buf->base);
            buffer->next = nullptr;
            recycleLink(buffer);
        }

        static void recycleLink(Buffer *buffer) {
            Buffer *oldHead = pool.head;
            pool.head = buffer;
            int size = 1;
            while (buffer->next) {
                buffer = buffer->next;
                ++size;
            }
            buffer->next = oldHead;
            pool.size += size;
            if (!oldHead) {
                pool.tail = buffer;
            }
        }

        static void recycleSingle(Buffer *buffer) {
            buffer->next = pool.head;
            pool.head = buffer;
            if (!pool.size) {
                pool.tail = buffer;
            }
            ++pool.size;
        }

    };

}


#endif //SKYNET_BYTEBUF_H
