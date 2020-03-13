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


    ResponseHandler::ResponseHandler(const shared_ptr<Channel> &ch) : ChannelHandler(ch) {
        bzero(tmpHead, sizeof(tmpHead));
    }

    int ResponseHandler::onMessage(IoBuf &buf) {
        Channel *channel = findTransferChannel(buf);
        if (channel) {
            channel->writeMsg(buf);
        }
        return 0;
    }

    ClientResponseHandler::ClientResponseHandler(const shared_ptr<Channel> &ch) : ResponseHandler(ch) {
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
        return nullptr;
    }

    ServerAppHandler::ServerAppHandler(const shared_ptr<Channel> &ch, vector<string> &&provideServs)
            : ResponseHandler(ch), provideServs(provideServs) {}

    ServerAppHandler::~ServerAppHandler() {
        Thread::local<Server>().removeServerAppChannel(ch, provideServs);
    }

    Channel *ServerAppHandler::findTransferChannel(IoBuf &buf) {
        return nullptr;
    }


}
