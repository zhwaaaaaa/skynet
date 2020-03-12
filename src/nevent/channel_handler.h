//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_CHANNEL_HANDLER_H
#define SKYNET_CHANNEL_HANDLER_H

#include "channel.h"
#include <memory>
#include <nevent/IoBuf.h>
#include <util/Convert.h>

namespace sn {
    using namespace std;
    using ChannelPtr = std::shared_ptr<Channel>;

    enum MsgType {
        MT_CONSUMER_SH = 1,
        MT_PROVIDER_SH = 2,
        MT_SH_RESP = 3,
        MT_HEARTBEAT_REQ = 4,
        MT_HEARTBEAT_RESP = 5,
        MT_REQUEST = 6,
        MT_RESPONSE = 7
    };

    //除了用于计算长度，别无用处
    PACK_STRUCT_START Request {
        uint8_t msgType;// msgType 等于固定值 MT_REQUEST
        uint32_t msgLen;
        uint32_t requestId;
        uint32_t clientId;
        uint32_t serverId;
        uint8_t servNameLen;
        // char serviceName[servNameLen]
        // uint8_t bodyType;
        // char body[msgLen-14-servNameLen];
    }PACK_STRUCT_END


    PACK_STRUCT_START Response {
        uint8_t msgType;// msgType 等于固定值 MT_REQUEST
        uint32_t msgLen;
        uint32_t requestId;
        uint32_t clientId;
        uint32_t serverId;
        uint8_t responseCode;
        uint8_t bodyType;
        char body[];
    }PACK_STRUCT_END


    constexpr const uint32_t RESP_HEADER_LEN = sizeof(Response);//19
    constexpr const uint32_t RESP_HEADER_CONTENT_LEN = sizeof(Response) - 1;//18

    constexpr const uint32_t REQ_SERVICE_NAME_LEN_OFFSET = offsetof(Request, servNameLen);
    constexpr const uint32_t REQ_CLIENT_ID_OFFSET = offsetof(Request, clientId);
    constexpr const uint32_t REQ_SERVER_ID_OFFSET = offsetof(Request, serverId);


    class ChannelHandler {
    protected:
        ChannelPtr ch;
        IoBuf ioBuf;
    public:
        explicit ChannelHandler(shared_ptr<Channel> ch);

        static void onChannelClosed(uv_handle_t *handle);

        static void onMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        static void onMessageArrived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

    public:
        virtual ~ChannelHandler() = default;


    protected:
        int onRead(ssize_t nread);

        virtual int onMessage(IoBuf &buf) = 0;

        virtual void onClose() {
            ch->close();
        }

        virtual void onError() {
            ch->close();
        }

    };


}


#endif //SKYNET_CHANNEL_HANDLER_H
