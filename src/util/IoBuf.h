//
// Created by dear on 3/5/20.
//

#ifndef SKYNET_IOBUF_H
#define SKYNET_IOBUF_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <glog/logging.h>

namespace sn {

    struct Block {
        uint32_t ref;
        Block *next;
        char buf[];
    };

    constexpr unsigned int BLOCK_SIZE = 65536;
    constexpr unsigned int BLOCK_DATA_LEN = BLOCK_SIZE - offsetof(Block, buf);

    class BlockPool {
    private:
        int maxSize;
        int current;
        Block *head;
    public:
        explicit BlockPool(int maxSize = 256) noexcept;

        ~BlockPool();

    public:
        static Block *get();

        static void recycle(Block *b);
    };


    class IoBuf {
        Block *head;
        Block *tail;
        uint32_t headOffset;
        uint32_t tailOffset;
        uint32_t size;
        uint32_t capacity;
    public:
        IoBuf() : head(nullptr), tail(nullptr), headOffset(0), tailOffset(0), size(0), capacity(0) {
        }

        /**
         * 获取写指针。和可写长度
         * 必须在clear或者回收之前，调用addSize报告写好的长度
         * @param buf
         * @param len
         */
        void writePtr(char **buf, uint32_t *len) {
            if (!tail) {
                head = tail = BlockPool::get();
                *buf = tail->buf;
                *len = BLOCK_DATA_LEN;
            } else {
                uint32_t left = BLOCK_DATA_LEN - tailOffset;
                *buf = tail->buf + tailOffset;
                *len = left;
            }
        }

        ~IoBuf() {
            clearAll();
        }

        /**
         * 清掉所有的Block。变成一个空结构
         */
        inline void clearAll() {
            CHECK(capacity == size);
            clear(size);
            head = tail = nullptr;
            CHECK(!tailOffset && !headOffset);
        }

        inline void clear(uint32_t clrSize) {
            CHECK(clrSize <= size);
            // 有数据的情况下 tailOffset
            if (head) {
                u_int32_t rLen = BLOCK_DATA_LEN - headOffset;
                Block *tmp = head;
                Block *next = tmp->next;
                while (clrSize > rLen) {
                    CHECK(next);
                    if (--tmp->ref) {
                        BlockPool::recycle(tmp);
                    }
                    tmp = next;
                    rLen += BLOCK_DATA_LEN;
                    next = tmp->next;
                }

                // 两个相等还有最后一个没回收
                if (clrSize == rLen) {
                    headOffset = 0;
                    head = next;
                    if (--tmp->ref) {
                        BlockPool::recycle(tmp);
                    }
                } else {
                    // 如果发现最后一个没有写完，是不会回收最后一个的
                    headOffset = BLOCK_DATA_LEN - (rLen - clrSize);
                    head = tmp;
                }
                size -= clrSize;
                capacity -= clrSize;
            } else {
                CHECK(!tailOffset && !headOffset);
            }

        }

        inline void addSize(uint32_t writed) {
            CHECK(tail);
            size += writed;
            tailOffset += writed;
            CHECK(tailOffset <= BLOCK_DATA_LEN);
            if (BLOCK_DATA_LEN == tailOffset) {
                tail = tail->next = BlockPool::get();
                tailOffset = 0;
            }
        }

        inline void extendBlock(uint32_t extSize = 1) {
            CHECK(extSize);
            capacity += extSize * BLOCK_DATA_LEN;
            if (!head) {
                head = tail = BlockPool::get();
                --extSize;
            }
            while (extSize--) {
                Block *b = BlockPool::get();
                tail->next = b;
                tail = b;
            }
        }

        /**
         * 修改IoBuffer中的值
         * @tparam T
         * @param v
         * @param offset
         * @return
         */
        template<typename T>
        void modifyData(const T &v, uint32_t offset = 0) {
            CHECK(size >= sizeof(T) + offset);
            Block *tmp = head;
            if (offset) {
                offset += headOffset;
                Block *next = tmp->next;
                while (offset >= BLOCK_DATA_LEN) {
                    CHECK(next);
                    tmp = next;
                    next = tmp->next;
                    offset -= BLOCK_DATA_LEN;
                }
            } else {
                offset = headOffset;
            }

            char *src = &v;
            if (sizeof(T) + offset <= BLOCK_DATA_LEN) {
                memcpy(tmp->buf + offset, src, sizeof(T));
                return;
            }

            uint32_t n = BLOCK_DATA_LEN - offset;
            memcpy(tmp->buf + offset, src, n);

            int t = n;
            int left = sizeof(T) - n;
            do {
                tmp = tmp->next;
                n = std::min(left, (int) BLOCK_DATA_LEN);
                memcpy(tmp->buf, src + t, n);
                t += n;
                left -= n;
            } while (left);
        }

        /**
         * 从开始截取一个IoBuf
         * @param buf
         * @param len
         */
        void popInto(IoBuf &buf, uint32_t len) {
            buf.clearAll();
            CHECK(len > 0 && len <= size);
            buf.head = head;
        }

        /**
         * 小心调用
         * 直接把指针指向buffer中一段内存。所以必须保证IoBuf被回收前 T 不再使用
         * @tparam T 纯结构体对象,如果对象处于不连续内存段，则t被设置为nullptr
         * @param t 指针引用
         * @param offset buf第一个字节便宜量
         */
        template<class T>
        inline void convertObj(T **t, uint32_t offset = 0) {
            CHECK(size >= offset + sizeof(T));
            Block *tmp = head;
            if (offset) {
                offset += headOffset;
                Block *next = tmp->next;
                while (offset >= BLOCK_DATA_LEN) {
                    CHECK(next);
                    tmp = next;
                    next = tmp->next;
                    offset -= BLOCK_DATA_LEN;
                }
            } else {
                offset = headOffset;
            }
            if (offset + sizeof(T) > BLOCK_DATA_LEN) {
                *t = nullptr;
            } else {
                *t = reinterpret_cast<T *>(tmp->buf + offset);
            }
        }

        /**
         * 会把接下来的一段内存拷贝到目标指针中
         *
         * @tparam T 结构体类型
         * @param t 内存地址
         * @param offset IoBuf偏移量
         */
        template<class T>
        void copyInto(T *v, uint32_t offset = 0) {
            CHECK(size >= sizeof(T) + offset);
            Block *tmp = head;
            if (offset) {
                offset += headOffset;
                Block *next = tmp->next;
                while (offset >= BLOCK_DATA_LEN) {
                    CHECK(next);
                    tmp = next;
                    next = tmp->next;
                    offset -= BLOCK_DATA_LEN;
                }
            } else {
                offset = headOffset;
            }

            char *dst = &v;
            if (sizeof(T) + offset <= BLOCK_DATA_LEN) {
                memcpy(dst, tmp->buf + offset, sizeof(T));
                return;
            }

            uint32_t n = BLOCK_DATA_LEN - offset;
            memcpy(dst, tmp->buf + offset, n);

            int t = n;
            int left = sizeof(T) - n;
            do {
                tmp = tmp->next;
                n = std::min(left, (int) BLOCK_DATA_LEN);
                memcpy(dst + t, tmp->buf, n);
                t += n;
                left -= n;
            } while (left);
        }

        /**
         * 从指定偏移位置读取一个类型，
         * 一般情况下这个函数用于读取小对象。否则会带来内存拷贝，影响性能
         * 大对象使用readObj或者copyInto
         * @tparam T 对象类型
         * @return t
         */
        template<typename T>
        inline T read(uint32_t offset = 0) {
            T t;
            coyInfo(&t, offset);
            return t;
        }


    };

}


#endif //SKYNET_IOBUF_H
