//
// Created by dear on 2020/2/6.
//

#ifndef SKYNET_TRANSFER_HANDLER_H
#define SKYNET_TRANSFER_HANDLER_H

#include "channel_handler.h"

namespace sn {

    // [1:serviceLen][0-238:serviceName][4:requestId][4:clientId][4:serverId][1:bodyType][4:bodyLen][0-4G:data]
    // 1+238+4+4+4+1+4+data

    using ServiceNamePtr = Segment<uint8_t> *;

    constexpr const uint MAX_SERVICE_LEN = 238;

    PACK_STRUCT_START HeaderId {
        // ServiceName service;
        // ==========================================
        uint32_t requestId;
        uint32_t clientId;
        uint32_t serverId;
        uint8_t bodyType;
        uint32_t bodyLen;
    }PACK_STRUCT_END

    constexpr const uint REQ_HEADER_LEN = sizeof(HeaderId);//17 [4:requestId][4:clientId][4:serverId][1:bodyType][4:bodyLen]
    class TransferHandler : public ChannelHandler {
    private:
        uint32_t firstBufferOffset;
        uint32_t lastBufferUsed;

        size_t readPkgLen;
        size_t packageLen;
        Buffer *lastReadBuffer;
        Buffer *firstReadBuffer;
        //buffer
        char tmpHead[256];// 用着256个字节来解决header粘包问题
    public:
        explicit TransferHandler(const shared_ptr<Channel> &ch);

    protected:
        void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) override;

        int onMessage(const uv_buf_t *buf, ssize_t nread) override;

        void onClose(const uv_buf_t *buf) override;

        void onError(const uv_buf_t *buf) override;

        virtual Channel *findOutCh(ServiceNamePtr serviceName) = 0;

        virtual Channel *setResponseChannelId(Header *header) = 0;

    };


    class ClientAppHandler : public ChannelHandler {
    private:
        ByteBuf byteBuf;
        Buffer *lastReadBuffer;
        size_t readedOffset;
        size_t decodeOffset;
        bool valid;
        uint readedBytes;
        uint requireBytes;
        // ===
        int packageLen;
        uint32_t readedPkg;
    public:
        explicit ClientAppHandler(const shared_ptr<Channel> &ch);


    protected:
        void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) override;

        int onMessage(const uv_buf_t *buf, ssize_t nread) override;

        void onClose(const uv_buf_t *buf) override;

        void onError(const uv_buf_t *buf) override;

    private:

    };


}


#endif //SKYNET_TRANSFER_HANDLER_H
