//
// Created by dear on 2020/2/5.
//

#include "client.h"
#include <memory>
#include <registry/naming_server.h>
#include <nevent/transfer_handler.h>
#include <nevent/io_error.h>
#include <nevent/stream_channel.h>


namespace sn {
    void Client::onNamingServerNotify(const string_view &serv, const vector<EndPoint> &eps,
                                      bool hashNext, void *param) {

        auto &client = Thread::local<Client>();

        for (const EndPoint ep:eps) {
            client.addServiceChannel(ep);
        }

        auto *serviceKeeper = static_cast<ServiceKeeper *>(param);
        if (hashNext) {
            serviceKeeper->servChanged(eps);
        } else {
            auto iterator = client.serviceMap.find(serv);
            if (iterator != client.serviceMap.end()) {
                // 需要删除malloc的key
                const char *keyVal = iterator->first.data();
                client.serviceMap.erase(iterator);
                free((void *) keyVal);
            }
        }
    }

    ServiceKeeper *Client::getByService(const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            return nullptr;
        }

        return iterator->second.get();
    }

    void Client::enableService(uint32_t channelId, const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator != serviceMap.end()) {
            iterator->second->care(channelId);
        } else {
            char *servCpy = static_cast<char *>(malloc(serv->len + 1));
            memcpy(servCpy, serv->buf, serv->len);
            servCpy[serv->len] = '\0';
            string_view x(servCpy, serv->len);

            auto y = std::make_shared<ServiceKeeper>(x);
            y->care(channelId);
            serviceMap.insert(make_pair(x, y));

            Thread::local<NamingServer>().subscribe(x, onNamingServerNotify, y.get());
        }
    }

    void Client::disableService(uint32_t channelId, const ServiceNamePtr serv) {
        string_view key(serv->buf, serv->len);
        auto iterator = serviceMap.find(key);
        if (iterator == serviceMap.end()) {
            LOG(WARNING) << "Not found service enabled:" << key;
            return;
        }

        if (iterator->second->noCare(channelId) && !iterator->second->count()) {
            LOG(INFO) << "disable not need service:" << key;
            Thread::local<NamingServer>().unsubscribe(key);
        }
    }

    void Client::addServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        if (iterator == channelMap.end()) {
            auto *pChannel = new TcpChannel<ServerReqHandler>;
            try {
                addLoopable(*pChannel);
                pChannel->connectTo(endPoint);
                // 这里先给一个nullptr。要连上了在handler中去set。
                channelMap.insert(make_pair(endPoint, std::make_shared<ChannelKeeper>(nullptr)));
            } catch (const IoError &e) {
                LOG(INFO) << e << " on connecting to " << endPoint;
                pChannel->close();
                delete pChannel;
            }
        }

    }

    shared_ptr<ChannelKeeper> Client::getServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        if (iterator == channelMap.end()) {
            return shared_ptr<ChannelKeeper>();
        }
        return iterator->second;
    }

    void Client::removeServiceChannel(const EndPoint endPoint) {
        auto iterator = channelMap.find(endPoint);
        if (iterator == channelMap.end()) {
            return;
        }
        if (!iterator->second->serviceCount()) {
            // TODO
        }

    }
}
