//
// Created by dear on 3/12/20.
//

#ifndef SKYNET_REQUESTHANDLER_H
#define SKYNET_REQUESTHANDLER_H

#include <cstdint>
#include <util/Convert.h>
#include <event/service_keeper.h>
#include "channel_handler.h"

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    constexpr const uint8_t MAX_SERVICE_LEN = 238;

    constexpr const uint8_t SKYNET_ERR_TRANSFER = 80;
    constexpr const uint8_t SKYNET_ERR_NO_SERVICE = 81;
    constexpr const uint8_t BODY_TYPE_STRING = 20;


    class RequestHandler : public ChannelHandler {
    private:
        //buffer
        char tmpHead[256];// 用着256个字节来解决header粘包问题
    public:
        explicit RequestHandler(const shared_ptr <Channel> &ch);

    protected:

        int onMessage(IoBuf &buf) override;

        virtual ChannelGroup *findOutCh(ServiceNamePtr serviceName) = 0;

        virtual void signMsg(IoBuf &buf) = 0;
    };


    class ClientAppHandler : public RequestHandler {
    private:
        vector<string> requiredService;
    public:
        explicit ClientAppHandler(const shared_ptr<Channel> &ch, vector<string> &&serv);

        ~ClientAppHandler() override;

    protected:

        ChannelGroup *findOutCh(ServiceNamePtr serviceName) override;

        void signMsg(IoBuf &buf) override;

    };


    class ServerReqHandler : public RequestHandler {
    public:
        explicit ServerReqHandler(const shared_ptr<Channel> &ch);

        ~ServerReqHandler() override;

    private:
        ChannelGroup *findOutCh(ServiceNamePtr serviceName) override;

        void signMsg(IoBuf &buf) override;
    };

}


#endif //SKYNET_REQUESTHANDLER_H
