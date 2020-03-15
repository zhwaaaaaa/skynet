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
#include <uv.h>

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
        int totalMalloc;
        int currentMalloc;
        Block *head;
    public:
        explicit BlockPool(int maxSize = 256) noexcept;

        ~BlockPool();

    public:
        static Block *get();

        static void recycle(Block *b);

        static void report();
    };


    class IoBuf {
        Block *head;
        Block *tail;
        uint32_t headOffset;
        uint32_t tailOffset;
        uint32_t size;
        uint32_t capacity;
    public:
//        IoBuf &operator=(IoBuf &) = delete;

    public:
        IoBuf() : head(nullptr), tail(nullptr), headOffset(0), tailOffset(0), size(0), capacity(0) {
        }

        IoBuf(IoBuf &&buf) : head(buf.head), tail(buf.tail),
                             headOffset(buf.headOffset), tailOffset(buf.tailOffset),
                             size(buf.size), capacity(buf.capacity) {
            buf.head = buf.tail = nullptr;
            buf.headOffset = buf.tailOffset = buf.size = buf.capacity = 0;
        }

        /**
         * 获取写指针。和可写长度
         * 必须在clear或者回收之前，调用addSize报告写好的长度
         * @param buf
         * @param len
         */
        void writePtr(char **buf, size_t *len) {
            uint32_t canWrite = capacity - size;
            if (!head || canWrite == 0) {
                extendBlock();
                *buf = tail->buf;
                *len = BLOCK_DATA_LEN;
            } else if (canWrite < 1024 && !size) {
                clearAll();
                extendBlock();
                *buf = tail->buf;
                *len = BLOCK_DATA_LEN;
            } else {
                *buf = tail->buf + tailOffset;
                *len = canWrite;
            }
        }

        ~IoBuf() {
            clearAll();
        }


        /**
         * 清掉所有的Block。变成一个空结构
         */
        inline void clearAll() {
            if (!head) {
                return;
            }
            int c = (int) (capacity + headOffset);
            Block *tmp = head;
            do {
                CHECK(tmp);
                Block *next = tmp->next;
                if (!--tmp->ref) {
                    BlockPool::recycle(tmp);
                }
                c -= BLOCK_DATA_LEN;
                tmp = next;
            } while (c > 0);
            head = tail = nullptr;
            headOffset = tailOffset = 0;
        }

        inline void clear(uint32_t clrSize) {
            CHECK(clrSize > 0 && clrSize <= size);
            // 有数据的情况下 tailOffset
            if (head) {
                u_int32_t rLen = BLOCK_DATA_LEN - headOffset;
                Block *tmp = head;
                Block *next = tmp->next;
                while (clrSize > rLen) {
                    CHECK(next);
                    if (!--tmp->ref) {
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
                    if (!--tmp->ref) {
                        BlockPool::recycle(tmp);
                    }
                } else {
                    // 如果发现最后一个没有写完，是不会回收最后一个的
                    headOffset = BLOCK_DATA_LEN - (rLen - clrSize);
                    head = tmp;
                }
                size -= clrSize;
                capacity -= clrSize;
                if (!capacity) {
                    CHECK(!head && !headOffset);
                    tail = nullptr;
                    tailOffset = 0;
                }
            } else {
                CHECK(!tailOffset && !headOffset);
            }

        }

        inline void addSize(uint32_t writed) {
            CHECK(tail);
            size += writed;
            tailOffset += writed;
            CHECK(tailOffset <= BLOCK_DATA_LEN);
        }

        inline void extendBlock() {
            if (!tail) {
                head = tail = BlockPool::get();
                return;
            } else if (tailOffset == BLOCK_DATA_LEN) {
                CHECK(capacity == size);
                Block *b = BlockPool::get();
                tail->next = b;
                tail = b;
            } else {
                int canWriteSize = (int) (capacity - size);
                // 还有一整个Block都没有使用不允许申请新的内存
                CHECK(canWriteSize > 0 && canWriteSize < BLOCK_DATA_LEN);
                tail->next = BlockPool::get();
            }
            capacity += BLOCK_DATA_LEN;
        }

        /**
        * 修改IoBuffer中的值
        * @tparam T
        * @param v
        * @param offset
        * @return
        */
        void modifyData(const void *ptr, uint32_t len, uint32_t offset = 0) {
            CHECK(size >= len + offset);
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

            const char *src = static_cast<const char *>(ptr);
            if (len + offset <= BLOCK_DATA_LEN) {
                memcpy(tmp->buf + offset, src, len);
                return;
            }

            uint32_t n = BLOCK_DATA_LEN - offset;
            memcpy(tmp->buf + offset, src, n);

            int t = n;
            int left = len - n;
            do {
                tmp = tmp->next;
                n = std::min(left, (int) BLOCK_DATA_LEN);
                memcpy(tmp->buf, src + t, n);
                t += n;
                left -= n;
            } while (left);
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

            const char *src = reinterpret_cast<const char *>(&v);
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
         * 从开始截取一个IoBuf.截取到的IoBuf不可再追加写数据
         * @param buf
         * @param len
         */
        void popInto(IoBuf &buf, uint32_t len) {
            buf.clearAll();
            CHECK(len > 0 && len <= size);
            buf.head = head;
            buf.size = buf.capacity = len;
            buf.headOffset = headOffset;

            uint32_t offset = len + headOffset;
            Block *tmp = head;
            Block *next = tmp->next;
            while (offset > BLOCK_DATA_LEN) {
                CHECK(next);
                tmp = next;
                next = tmp->next;
                offset -= BLOCK_DATA_LEN;
            }
            buf.tailOffset = offset;
            buf.tail = tmp;

            if (offset == BLOCK_DATA_LEN) {
                head = next;
                headOffset = 0;
            } else {
                head = tmp;
                headOffset = buf.tailOffset;
                ++tmp->ref;
            }
            size -= len;
            capacity -= len;
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

        inline void *convertOrCopyLen(uint32_t len, void *ptr, uint32_t offset = 0) {
            CHECK(size >= offset + len);
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

            if (offset + len > BLOCK_DATA_LEN) {
                char *dst = static_cast<char *>(ptr);
                uint32_t n = BLOCK_DATA_LEN - offset;
                memcpy(dst, tmp->buf + offset, n);

                int t = n;
                int left = (int) (len - n);
                do {
                    tmp = tmp->next;
                    n = std::min(left, (int) BLOCK_DATA_LEN);
                    memcpy(dst + t, tmp->buf, n);
                    t += n;
                    left -= n;
                } while (left);
                return ptr;
            } else {
                return tmp->buf + offset;
            }
        }

        inline void copyIntoPtr(uint32_t len, void *ptr, uint32_t offset = 0) {
            CHECK(size >= offset + len);
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

            if (offset + len > BLOCK_DATA_LEN) {
                char *dst = static_cast<char *>(ptr);
                uint32_t n = BLOCK_DATA_LEN - offset;
                memcpy(dst, tmp->buf + offset, n);

                int t = n;
                int left = (int) (len - n);
                do {
                    tmp = tmp->next;
                    n = std::min(left, (int) BLOCK_DATA_LEN);
                    memcpy(dst + t, tmp->buf, n);
                    t += n;
                    left -= n;
                } while (left);
            } else {
                memcpy(ptr, tmp->buf + offset, len);
            }
        }

        inline uint32_t getSize() {
            return size;
        }

        inline uint32_t getCapacity() {
            return capacity;
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

            char *dst = reinterpret_cast<char *>(v);
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
            copyInto<T>(&t, offset);
            return t;
        }

        /**
         * 因为调用比较频繁，提供快捷方法。
         * @return
         */
        inline uint8_t readUint8() {
            return (uint8_t) head->buf[headOffset];
        }

        /**
         * 因为调用比较频繁，提供快捷方法。不进行长度检查。由调用者执行判断
         * @return
         */
        inline uint8_t readUint8(uint32_t offset) {
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
            return (uint8_t) head->buf[offset];
        }

        uint32_t dataBlockSize() const {
            uint32_t sizeAndHead = headOffset + size;
            auto i = sizeAndHead / BLOCK_DATA_LEN;
            return tailOffset ? i + 1 : i;
        }

        void dataPtr(uv_buf_t *uvBuf, size_t len) const {
            CHECK(len > 1);
            uvBuf->base = head->buf + headOffset;
            uvBuf->len = BLOCK_DATA_LEN - headOffset;

            Block *tmp = head->next;
            auto lastIndex = len - 1;
            for (int i = 1; i < lastIndex; ++i) {
                CHECK(tmp);
                uvBuf[i].base = tmp->buf;
                uvBuf[i].len = BLOCK_DATA_LEN;
                tmp = tmp->next;
            }
            CHECK(tmp);
            uvBuf[lastIndex].base = tmp->buf;
            uvBuf[lastIndex].len = tailOffset;
        }

        void firstDataPtr(uv_buf_t &uvBuf) const {
            uvBuf.base = head->buf + headOffset;
            uvBuf.len = BLOCK_DATA_LEN - headOffset;
        }

        template<class T>
        void write(const T &val) {
            const char *src = reinterpret_cast<const char *>(&val);
            char *p;
            size_t canWriteSize;

            int cped = 0;
            int left = (int) sizeof(T);
            do {
                writePtr(&p, &canWriteSize);
                int cp = std::min(left, (int) canWriteSize);
                memcpy(p, src + cped, cp);
                left -= cp;
                cped += cp;
            } while (left > 0);

            CHECK(left == 0);
            size += sizeof(T);
        }

        void write(const void *ptr, size_t len) {
            CHECK(ptr && len);

            const char *src = static_cast<const char *>(ptr);
            char *p;
            size_t canWriteSize;

            int cped = 0;
            int left = (int) len;
            do {
                writePtr(&p, &canWriteSize);
                int cp = std::min(left, (int) canWriteSize);
                memcpy(p, src + cped, cp);
                left -= cp;
                cped += cp;
            } while (left > 0);

            CHECK(left == 0);
            size += len;
        }

    };

}


#endif //SKYNET_IOBUF_H
