//
// Created by dear on 2020/2/5.
//

#include "client.h"
#include <memory>
#include <utility>
#include <registry/naming_server.h>
#include <nevent/ResponseHandler.h>
#include <nevent/io_error.h>
#include <nevent/TcpChannel.h>


namespace sn {

    struct ServiceChangeContext {
        Client *client;
        string service;
        vector<string> epstr;

        ServiceChangeContext(Client *client, const string_view &service, vector<string> epstr)
                : client(client), service(service), epstr(std::move(epstr)) {}
    };

    void namingServerNotify(void *p) {
        ServiceChangeContext *ctx = static_cast<ServiceChangeContext *>(p);
        vector<EndPoint> eps;
        eps.reserve(ctx->epstr.size());
        for (const string &str:ctx->epstr) {
            EndPoint ep;
            str2endpoint(str.c_str(), &ep);
            ctx->client->addServiceChannel(ep);
            eps.push_back(ep);
        }

        ServiceKeeper *serviceKeeper = ctx->client->getByService(ctx->service);
        if (serviceKeeper) {
            serviceKeeper->servChanged(eps);
        }
        /* auto iterator = client.serviceMap.find(serv);
         if (iterator != client.serviceMap.end()) {
             // 取消订阅的时候回调函数是同步执行的。所以取消订阅的时候service一定没有人用了。
             CHECK(!iterator->second->count());
             // 需要删除malloc的key
             const char *keyVal = iterator->first.data();
             client.serviceMap.erase(iterator);
             free((void *) keyVal);
         }*/
        delete ctx;
    }

    void Client::onNamingServerNotify(const string_view &serv, const vector<string> &epstr, void *param) {
        auto *client = static_cast<Client *>(param);
        client->sendEvent(namingServerNotify, new ServiceChangeContext(client, serv, epstr));
    }

    ServiceKeeper *Client::getByService(const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            return nullptr;
        }

        return iterator->second.get();
    }

    ServiceKeeper *Client::getByService(const string_view &key) {
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            return nullptr;
        }

        return iterator->second.get();
    }


    /**
     * 这里要完成2件事情：
     * 1.确认要订阅naming server的服务。
     * 2.尽可能要连接上目标服务器
     * @param channelId
     * @param key
     * @return
     */
    int Client::enableService(uint32_t channelId, const string_view &key) {
        auto iterator = serviceMap.find(key);
        if (iterator != serviceMap.end()) {
            iterator->second->care(channelId);
            return iterator->second->count();
        } else {
            char *servCpy = static_cast<char *>(malloc(key.size() + 1));
            memcpy(servCpy, key.data(), key.size());
            servCpy[key.size()] = '\0';
            string_view x(servCpy, key.size());

            auto y = std::make_shared<ServiceKeeper>(x);
            y->care(channelId);
            serviceMap.insert(make_pair(x, y));

            auto ptr = namingServer->subscribe(x, onNamingServerNotify, this);
            vector<EndPoint> eps;
            eps.reserve(ptr->size());
            for_each(ptr->begin(), ptr->end(), [&](const string &str) {
                EndPoint ep;
                str2endpoint(str.c_str(), &ep);
                addServiceChannel(ep);
                eps.push_back(ep);
            });

            y->servChanged(eps);
            return eps.size();
        }
    }

    int Client::disableService(uint32_t channelId, const string_view &key) {
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            LOG(WARNING) << "Not found service enabled:" << key;
            return 0;
        }
        iterator->second->noCare(channelId);
        auto i = iterator->second->count();
        if (!i) {
            // service 不再需要了
            LOG(INFO) << "disable not need service:" << key;
            const char *c = iterator->first.data();
            serviceMap.erase(iterator);
            // 应该用定时器延时注销
            namingServer->unsubscribe(key);
            free((void *) c);
        }
        return i;
    }

    void Client::addServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        if (iterator == channelMap.end()) {
            // 这里先给一个nullptr。要连上了在handler中去set。
            channelMap.insert(make_pair(endPoint, std::make_shared<ChannelKeeper>(nullptr)));
            auto *pChannel = new TcpChannel<ClientResponseHandler>;
            try {
                addLoopable(*pChannel);
                pChannel->connectTo(endPoint);
            } catch (const IoError &e) {
                LOG(ERROR) << " on connecting to " << endPoint << " " << e;
                pChannel->close();
                delete pChannel;
            }
        }

    }

    shared_ptr<ChannelKeeper> Client::getServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        CHECK(iterator != channelMap.end());
        return iterator->second;
    }

    void Client::removeServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        if (iterator == channelMap.end()) {
            return;
        }
        if (iterator->second->serviceCount()) {
            // 降低channel引用计数。以便于Channel的内存回收
            iterator->second->channelPointer().reset();
            // 这个channel还有service需要使用
            // 所以必须重连这个Channel
            LOG(INFO) << "连接" << endPoint << " 还承载了服务，重连...";
            auto *pChannel = new TcpChannel<ClientResponseHandler>;
            try {
                addLoopable(*pChannel);
                pChannel->connectTo(endPoint);
            } catch (const IoError &e) {
                LOG(ERROR) << " on connecting to " << endPoint << " " << e;
                pChannel->close();
                delete pChannel;
            }
        } else {
            // 这个channel已经没有service使用了
            channelMap.erase(iterator);
        }

    }

    Channel *Client::getResponseChannel(uint32_t channelId) {
        auto iterator = responseChs.find(channelId);
        if (iterator != responseChs.end()) {
            return iterator->second.get();
        }
        return nullptr;
    }

    void Client::addResponseChannel(ChannelPtr &respCh) {
        responseChs.insert(make_pair(respCh->channelId(), respCh));
    }

    void Client::removeResponseChannel(uint32_t channelId) {
        auto iterator = responseChs.find(channelId);
        CHECK(iterator != responseChs.end());
        responseChs.erase(iterator);
    }

    Client::Client(NamingServer &namingServer) : namingServer(&namingServer) {}

    void Client::onLoopStart() {
        Thread::local<Client>(this);
    }

    void Client::onLoopStop() {
        LOG(INFO)<<"Client closing....";
        for(const auto & p:responseChs){
            p.second->close();
        }
        responseChs.clear();
        LOG(INFO)<<"Client closed";
        Thread::localRelease<Client>();
    }

}
