//
// Created by dear on 2020/2/10.
//

#ifndef SKYNET_SHAKE_HANDS_HANDLER_H
#define SKYNET_SHAKE_HANDS_HANDLER_H

#include "channel_handler.h"
#include <event/client.h>


namespace sn {
    class ShakeHandsHandler : public ChannelHandler {
    protected:
        // =====
        int serviceSize;
    public:
        explicit ShakeHandsHandler(const shared_ptr<Channel> &ch);

    protected:
        int onMessage(IoBuf &buf) override;

        virtual int doShakeHands(vector<string> &services) = 0;
    };


    class ClientShakeHandsHandler : public ShakeHandsHandler {
    public:
        explicit ClientShakeHandsHandler(const shared_ptr<Channel> &ch);

    protected:
        int doShakeHands(vector<string> &services) override;
    };


    class ServerShakeHandsHandler : public ShakeHandsHandler {
    public:
        explicit ServerShakeHandsHandler(const shared_ptr<Channel> &ch);

    protected:
        int doShakeHands(vector<string> &services) override;
    };
}


#endif //SKYNET_SHAKE_HANDS_HANDLER_H
