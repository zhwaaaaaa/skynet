//
// Created by dear on 3/12/20.
//

#ifndef SKYNET_CONVERT_H
#define SKYNET_CONVERT_H

#if defined(__GNUC__)
#define PACK_STRUCT_START struct __attribute__((__packed__))
#define PACK_STRUCT_END ;
#else
#define PACK_STRUCT_START #pragma pack(push, _packed, 1)
#define PACK_STRUCT_END ; #pragma pack(pop, _packed)
#endif


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
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=(val)

//=======================================================
#define CONVERT_VAL_64(val) __builtin_bswap64(val)
#define CONVERT_VAL_32(val) __builtin_bswap31(val)
#define CONVERT_VAL_16(val) __builtin_bswap16(val)
#define CONVERT_VAL_8(val) val

#else //小端模式则什么也不做直接返回
#define SEGMENT_LEN_64(segment) (segment)->len
#define SEGMENT_LEN_32(segment) (segment)->len
#define SEGMENT_LEN_16(segment) (segment)->len
#define SEGMENT_LEN_8(segment) (segment)->len

//================================================

#define WRITELE_VAL_64(ptr, val) *reinterpret_cast<uint64_t *>(ptr)=(val)
#define WRITELE_VAL_32(ptr, val) *reinterpret_cast<uint32_t *>(ptr)=(val)
#define WRITELE_VAL_16(ptr, val) *reinterpret_cast<uint16_t *>(ptr)=(val)
#define WRITELE_VAL_8(ptr, val) *reinterpret_cast<uint8_t *>(ptr)=(val)

#define CONVERT_VAL_64(val) (val)
#define CONVERT_VAL_32(val) (val)
#define CONVERT_VAL_16(val) (val)
#define CONVERT_VAL_8(val)  (val)
#endif

namespace sn {


    template<typename LEN>
    PACK_STRUCT_START Segment {
        const LEN len;
        char buf[];

        template<typename NEW_LEN = LEN>
        Segment<NEW_LEN> *sub(LEN offset = 0) {
            return reinterpret_cast<Segment<NEW_LEN> *>(buf + offset);
        }

        template<typename T = Segment<LEN>>
        T *last() {
            return reinterpret_cast<T *>(buf + len);
        }
    }PACK_STRUCT_END

}

#endif //SKYNET_CONVERT_H
