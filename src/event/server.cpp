//
// Created by dear on 2020/2/13.
//

#include <cassert>
#include "server.h"

namespace sn {


    void Server::addServerAppChannel(ChannelPtr &channelPtr, vector<ServiceDesc> &sds) {

        for (const ServiceDesc &sd:sds) {
            string_view key(sd.name->buf, sd.name->len);
            auto iterator = serverAppChs.find(key);
            if (iterator != serverAppChs.end()) {
                auto i = iterator->second->addChannel(channelPtr);
                assert(i > 1);
            } else {
                // 之前没有提供过相应的服务
                char *servName = static_cast<char *>(malloc(sd.name->len + 1));
                memcpy(servName, sd.name->buf, sd.name->len);
                servName[sd.name->len] = '\0';
                string_view x(servName, sd.name->len);
                serverAppChs.insert(make_pair(x, make_shared<ChannelHolder>(channelPtr)));

                //TODO register to etcd
            }
        }
    }

    void Server::removeServerAppChannel(ChannelPtr &channelPtr, vector<string> &servNames) {
        for (const string &serv:servNames) {
            auto iterator = serverAppChs.find(serv);
            if (iterator != serverAppChs.end()) {
                auto i = iterator->second->removeChannel(channelPtr);
                if (i == 0) {
                    const char *key = iterator->first.data();
                    serverAppChs.erase(iterator);
                    free((void *) key);
                }
            }
        }
    }

    ChannelHolder *Server::getChannelByService(const string_view &serv) {
        auto iterator = serverAppChs.find(serv);
        if (iterator != serverAppChs.end()) {
            return iterator->second.get();
        }
        return nullptr;
    }

    Channel *Server::getResponseChannel(uint32_t channelId) {
        auto iterator = responseChs.find(channelId);
        if (iterator != responseChs.end()) {
            return iterator->second.get();
        }
        return nullptr;
    }

    void Server::addResponseChannel(ChannelPtr &respCh) {
        responseChs.insert(make_pair(respCh->channelId(), respCh));
    }

    void Server::removeResponseChannel(uint32_t channelId) {
        auto iterator = responseChs.find(channelId);
        assert(iterator != responseChs.end());
        responseChs.erase(iterator);
    }

}