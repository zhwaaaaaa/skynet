//
// Created by dear on 3/12/20.
//

#include <thread/thread.h>
#include <event/client.h>
#include <event/server.h>
#include "RequestHandler.h"

namespace sn {
    RequestHandler::RequestHandler(const shared_ptr<Channel> &ch)
            : ChannelHandler(ch) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    int RequestHandler::onMessage(IoBuf &buf) {

        uint8_t servNameLen = ioBuf.readUint8(REQ_SERVICE_NAME_LEN_OFFSET);
        ServiceNamePtr serviceName = static_cast<ServiceNamePtr>(
                ioBuf.convertOrCopyLen(servNameLen + 1, tmpHead, REQ_SERVICE_NAME_LEN_OFFSET));

        // find service and send msg;
        ChannelGroup *serviceKeeper = findOutCh(serviceName);
        int status = -1;
        if (serviceKeeper) {
            signMsg(buf);
            auto serviceSize = serviceKeeper->channelSize();
            for (int i = 0; i < serviceSize; ++i) {
                auto pChannel = serviceKeeper->nextChannel();
                if (pChannel) {
                    status = pChannel->writeMsg(buf);
                    if (status != -1) {
                        break;
                    }
                }
            }
        }
        if (status < 0) {
            IoBuf r;
            r.extendBlock();
            size_t maxLen;
            char *c;
            r.writePtr(&c, &maxLen);
            auto *resp = reinterpret_cast<Response *>(c);
            resp->msgType = MT_RESPONSE;
            // data 数据为0
            resp->msgLen = RESP_HEADER_LEN;
            // 把requestId clientId serverId copy到返回buffer中
            buf.copyIntoPtr(12, c + 5, 5);
            resp->responseCode = status == -1 ? CONVERT_VAL_8(SKYNET_ERR_NO_SERVICE)
                                              : CONVERT_VAL_8(SKYNET_ERR_TRANSFER);
            resp->bodyType = 0;
            ch->writeMsg(r);
        }
        return 0;
    }



    ClientAppHandler::ClientAppHandler(const shared_ptr<Channel> &ch, vector<string> &&serv)
            : RequestHandler(ch), requiredService(serv) {
        Thread::local<Client>().addResponseChannel(this->ch);
    }

    ChannelGroup *ClientAppHandler::findOutCh(ServiceNamePtr serviceName) {
        return Thread::local<Client>().getByService(serviceName);
    }

    void ClientAppHandler::signMsg(IoBuf &buf) {
        buf.modifyData(ch->channelId(), REQ_CLIENT_ID_OFFSET);
    }

    ClientAppHandler::~ClientAppHandler() {
        Thread::local<Client>().removeResponseChannel(ch->channelId());
        Client &client = Thread::local<Client>();
        for (const auto &s:requiredService) {
            client.disableService(ch->channelId(), s);
            LOG(INFO) << ch->channelId() << "with ClientAppHandler not require service " << s;
        }
    }


    ServerReqHandler::ServerReqHandler(const shared_ptr<Channel> &ch) : RequestHandler(ch) {
        Thread::local<Server>().addResponseChannel(this->ch);
    }


    ChannelGroup *ServerReqHandler::findOutCh(ServiceNamePtr serviceName) {
        return Thread::local<Server>().getChannelByService(string_view(serviceName->buf, serviceName->len));
    }

    void ServerReqHandler::signMsg(IoBuf &buf) {
        buf.modifyData(ch->channelId(), REQ_SERVER_ID_OFFSET);
    }

    ServerReqHandler::~ServerReqHandler() {
        Thread::local<Server>().removeResponseChannel(ch->channelId());
    }

}
