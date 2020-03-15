//
// Created by dear on 2020/2/6.
//

#include "ResponseHandler.h"
#include <glog/logging.h>
#include <thread/thread.h>
#include <event/client.h>
#include <event/server.h>
#include "stream_channel.h"
#include "TcpChannel.h"

namespace sn {


    ResponseHandler::ResponseHandler(shared_ptr<Channel> &ch) : ChannelHandler(ch) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    int ResponseHandler::onMessage(IoBuf &buf) {
        Channel *channel = findTransferChannel(buf);
        if (channel) {
            channel->writeMsg(buf);
        }
        return 0;
    }

    ClientResponseHandler::ClientResponseHandler(ChannelPtr &ch) : ResponseHandler(ch) {
        // client response handler负责转发server的的返回。
        // 但它也负责写ClientAppHandler转发的请求
        // 所以要注册自己的channel到Client上。以便于ClientAppHandler可以使用
        // 因为对于客户端来说，它需要连到外部的channel上去。所以它一定是TcpChannel
        TcpChannel<ClientResponseHandler> *channel = dynamic_cast<TcpChannel<ClientResponseHandler> *>(ch.get());
        auto point = channel->remoteAddr();
        LOG(INFO) << "ClientResponseHandler() 连到server：" << point;
        Thread::local<Client>().getServiceChannel(point)->resetChannel(this->ch);
    }

    ClientResponseHandler::~ClientResponseHandler() {
        // 当连接断掉了
        TcpChannel<ClientResponseHandler> *channel = dynamic_cast<TcpChannel<ClientResponseHandler> *>(ch.get());
        auto point = channel->remoteAddr();
        LOG(INFO) << "~ClientResponseHandler() 断开server：" << point;
        Thread::local<Client>().removeServiceChannel(point);
    }

    Channel *ClientResponseHandler::findTransferChannel(IoBuf &buf) {
        uint32_t val;
        auto *ptr = static_cast<uint32_t *>(buf.convertOrCopyLen(sizeof(val), &val, RESP_CLIENT_ID_OFFSET));
        return Thread::local<Server>().getResponseChannel(*ptr);
    }

    ServerAppHandler::ServerAppHandler(shared_ptr<Channel> &ch, vector<string> &services)
            : ResponseHandler(ch) {
        auto &server = Thread::local<Server>();
        IoBuf buf;
        buf.write<uint8_t>(MT_SH_RESP);
        buf.write<uint32_t>(0);
        buf.write<uint16_t>((uint16_t) CONVERT_VAL_16(services.size()));
        uint32_t pckLen = 2;
        for (const auto &serv:services) {
            buf.write((uint8_t) (0xFF & serv.size()));
            int len = server.addServerAppChannel(this->ch, serv);
            buf.write(serv.data(), serv.size());
            buf.write((uint8_t) len);
            pckLen += len + 2;
            provideServs.push_back(serv);
        }
        buf.modifyData<uint32_t>(pckLen, 1);
        ch->writeMsg(buf);
    }

    ServerAppHandler::~ServerAppHandler() {
        for (const auto &serv:provideServs) {
            Thread::local<Server>().removeServerAppChannel(ch, serv);
        }
    }

    Channel *ServerAppHandler::findTransferChannel(IoBuf &buf) {
        uint32_t val;
        auto *ptr = static_cast<uint32_t *>(buf.convertOrCopyLen(sizeof(val), &val, RESP_SERVER_ID_OFFSET));
        return Thread::local<Server>().getResponseChannel(*ptr);
    }


}
