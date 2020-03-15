//
// Created by dear on 2020/2/6.
//

#ifndef SKYNET_RESPONSEHANDLER_H
#define SKYNET_RESPONSEHANDLER_H

#include <event/service_keeper.h>
#include "channel_handler.h"
#include <nevent/IoBuf.h>
#include <util/Convert.h>

namespace sn {


    class ResponseHandler : public ChannelHandler {
    private:
        //buffer
        char tmpHead[sizeof(Response)];// 用着sizeof(ResponseId)个字节来解决header粘包问题
    public:
        explicit ResponseHandler(ChannelPtr &ch);

    protected:
        int onMessage(IoBuf &buf) override;

        virtual Channel *findTransferChannel(IoBuf &buf) = 0;
    };

    /**
     * Client 转发Server返回的数据
     */
    class ClientResponseHandler : public ResponseHandler {
    public:
        explicit ClientResponseHandler(ChannelPtr &ch);

        ~ClientResponseHandler() override;

    protected:
        Channel *findTransferChannel(IoBuf &buf) override;
    };

    /**
     * Server转发服务接受方返回的数据，服务器连上来的
     */
    class ServerAppHandler : public ResponseHandler {
    private:
        vector<string> provideServs;
    public:
        ServerAppHandler(ChannelPtr &ch, vector<string> &provideServs);

        ~ServerAppHandler() override;

    protected:
        Channel *findTransferChannel(IoBuf &buf) override;
    };


}


#endif //SKYNET_RESPONSEHANDLER_H
