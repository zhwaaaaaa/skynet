//
// Created by dear on 2020/2/10.
//

#ifndef SKYNET_SHAKE_HANDS_HANDLER_H
#define SKYNET_SHAKE_HANDS_HANDLER_H

#include "channel_handler.h"
#include <event/client.h>


namespace sn {

    PACK_STRUCT_START ShakeHandsHeader {
        uint32_t len;
        uint8_t shakeType;
        uint16_t serviceSize;
        char buf[];

        ServiceNamePtr last() {
            return reinterpret_cast<ServiceNamePtr>(buf);
        }
    }PACK_STRUCT_END;

    constexpr const uint8_t SERVICE_TYPE_CLIENT = 0;
    constexpr const uint8_t SERVICE_TYPE_SERVER = 0xFF;


    class ClientShakeHandsHandler : public ChannelHandler {
    private:
        bool shakeComplete;
        uint32_t readBytes;
        Buffer *firstBuffer;
        Buffer *lastBuffer;
        uint32_t lastBufferOffset;
        // =====
        uint32_t shakePkgLen;
        int serviceSize;

    public:
        explicit ClientShakeHandsHandler(const shared_ptr<Channel> &ch);

        ~ClientShakeHandsHandler() override;

    protected:
        void onMemoryRequired(size_t suggested_size, uv_buf_t *buf) override;

        int onMessage(const uv_buf_t *buf, ssize_t nread) override;

        void onClose(const uv_buf_t *buf) override;

        void onError(const uv_buf_t *buf) override;


    };
}


#endif //SKYNET_SHAKE_HANDS_HANDLER_H
