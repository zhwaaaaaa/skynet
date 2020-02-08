//
// Created by dear on 20-1-26.
//

#ifndef SKYNET_BUFFER_H
#define SKYNET_BUFFER_H


#include <glob.h>
#include <cstdlib>
#include <iostream>

namespace sn {

    template<typename LEN>
    struct __attribute__ ((__packed__)) Segment {
        const LEN len;
        char buf[];

        template<typename NEW_LEN = LEN>
        Segment<NEW_LEN> *sub(LEN offset = 0) {
            return reinterpret_cast<Segment<NEW_LEN> *>(buf + offset);
        }

    };

    struct BufStr {
        uint len;
        const char *buf;

        template<typename LEN>
        BufStr(const Segment<LEN> &s) noexcept {
            len = s.len;
            buf = s.buf;
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const sn::BufStr &s) {
        return os << "[" << s.len << "]" << (s.buf, s.len);
    }
}

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
//当系统为大端时，把face_code结构体中的以小端模式存储的数据转换为大端
#define SEGMENT_LEN_64(segment) __builtin_bswap64((segment)->len)
#define SEGMENT_LEN_32(segment) __builtin_bswap32((segment)->len)
#define SEGMENT_LEN_16(segment) __builtin_bswap16((segment)->len)
#define SEGMENT_LEN_8(segment) (segment)->len

//================================================
#define WRITELE_VAL_64(ptr, val) *reinterpret_cast<uint64_t *>(ptr)=__builtin_bswap64(val)
#define WRITELE_VAL_32(ptr, val) *reinterpret_cast<uint32_t *>(ptr)=__builtin_bswap32(val)
#define WRITELE_VAL_16(ptr, val) *reinterpret_cast<uint16_t *>(ptr)=__builtin_bswap16(val)
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=val

#else //小端模式则什么也不做直接返回
#define SEGMENT_LEN_64(segment) (segment)->len
#define SEGMENT_LEN_32(segment) (segment)->len
#define SEGMENT_LEN_16(segment) (segment)->len
#define SEGMENT_LEN_8(segment) (segment)->len

//================================================

#define WRITELE_VAL_64(ptr, val) *reinterpret_cast<uint64_t *>(ptr)=val
#define WRITELE_VAL_32(ptr, val) *reinterpret_cast<uint32_t *>(ptr)=val
#define WRITELE_VAL_16(ptr, val) *reinterpret_cast<uint16_t *>(ptr)=val
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=val

#define READ_VAL_64(fPtr, fLen, sPtr)
#endif


namespace sn {

    struct SegmentRef {
        uint offset;
        uint len;
        char *buf;
        SegmentRef *prev;
        SegmentRef *next;
    };
    static thread_local SegmentRef *cache;// thread local

    struct Buffer {
        int refCount;
        Buffer *next;
        char buf[];
    };

    constexpr unsigned int BUFFER_PAD = offsetof(Buffer, buf);
    constexpr unsigned int BUFFER_SIZE = 65536;
    constexpr unsigned int BUFFER_BUF_LEN(BUFFER_SIZE - BUFFER_PAD);

}

#define BUF_TO_BUFFER(ch) reinterpret_cast<Buffer *>(ch - BUFFER_PAD)


#endif //MESHER_BUFFER_H
