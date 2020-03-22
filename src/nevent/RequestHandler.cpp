//
// Created by dear on 3/12/20.
//

#include <thread/thread.h>
#include <event/client.h>
#include <event/server.h>
#include "RequestHandler.h"

namespace sn {
    RequestHandler::RequestHandler(ChannelPtr &ch)
            : ChannelHandler(ch) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    int RequestHandler::onMessage(IoBuf &buf) {
        uint8_t msgType = buf.readUint8();
        if (msgType != MT_REQUEST) {
            return -1;
        }

        uint8_t servNameLen = buf.readUint8(REQ_SERVICE_NAME_LEN_OFFSET);
        ServiceNamePtr serviceName = static_cast<ServiceNamePtr>(
                buf.convertOrCopyLen(servNameLen + 1, tmpHead, REQ_SERVICE_NAME_LEN_OFFSET));

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
            size_t maxLen;
            char *c;
            r.writePtr(&c, &maxLen);
            auto *resp = reinterpret_cast<Response *>(c);
            resp->msgType = MT_RESPONSE;
            // data 数据为0
            resp->msgLen = RESP_HEADER_LEN - 5;
            // 把requestId clientId serverId copy到返回buffer中
            buf.copyIntoPtr(12, c + 5, 5);
            resp->responseCode = status == -1 ? CONVERT_VAL_8(SKYNET_ERR_NO_SERVICE)
                                              : CONVERT_VAL_8(SKYNET_ERR_TRANSFER);
            resp->bodyType = 0;
            r.addSize(RESP_HEADER_LEN);
            ch->writeMsg(r);
        }
        return 0;
    }


    ClientAppHandler::ClientAppHandler(shared_ptr<Channel> &ch, vector<string> &serv)
            : RequestHandler(ch) {
        auto &client = Thread::local<Client>();
        IoBuf buf;
        buf.write<uint8_t>(MT_SH_RESP);
        buf.write<uint32_t>(0);
        buf.write<uint16_t>((uint16_t) CONVERT_VAL_16(serv.size()));
        uint32_t pckLen = 2;
        for (const auto &serv:serv) {
            buf.write((uint8_t) (0xFF & serv.size()));
            int len = client.enableService(ch->channelId(), serv);
            buf.write(serv.data(), serv.size());
            buf.write<uint8_t>(0xFF & len);
            pckLen += serv.size() + 2;
            requiredService.push_back(serv);
        }
        buf.modifyData<uint32_t>(pckLen, 1);
        ch->writeMsg(buf);
        client.addResponseChannel(this->ch);
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


    ServerReqHandler::ServerReqHandler(ChannelPtr &ch) : RequestHandler(ch) {
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
