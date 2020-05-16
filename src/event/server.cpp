//
// Created by dear on 2020/2/13.
//

#include <cassert>
#include "server.h"

namespace sn {
    ChannelHolder::ChannelHolder() : index(0) {
        LOG(INFO) << "ChannelHolder()";
    }

    ChannelHolder::~ChannelHolder() {
        LOG(INFO) << "~ChannelHolder()";
    }

    ChannelHolder::ChannelHolder(ChannelPtr &first) : index(0) {
        chs.push_back(first);
    }

    int ChannelHolder::addChannel(ChannelPtr &ch) {

        for (const ChannelPtr &c:chs) {
            if (c->channelId() == ch->channelId()) {
                CHECK(false) << "channel added in channel hoder";
            }
        }
        chs.push_back(ch);
        return chs.size();
    }

    int ChannelHolder::removeChannel(ChannelPtr &ch) {
        auto iterator = remove_if(chs.begin(), chs.end(), [&](ChannelPtr &c) {
            return c->channelId() == ch->channelId();
        });

        chs.erase(iterator);
        return chs.size();
    }

    Channel *ChannelHolder::nextChannel() {
        return chs[++index % chs.size()].get();
    }

    int ChannelHolder::removeChannel(uint32_t channelId) {
        auto iterator = remove_if(chs.begin(), chs.end(), [&](ChannelPtr &c) {
            return c->channelId() == channelId;
        });

        chs.erase(iterator);
        return chs.size();
    }

    Server::Server(NamingServer &server) : namingServer(&server) {
    }

    void Server::onLoopStart() {
        Thread::local<Server>(this);
    }

    void Server::onLoopStop() {
        LOG(INFO) << "Server closing";

        for(const auto &p: serverAppChs){
            namingServer->unregisterService(p.first);
        }
        serverAppChs.clear();
        LOG(INFO) << "Server closed";
        Thread::localRelease<Server>();
    }


    int Server::addServerAppChannel(ChannelPtr &channelPtr, const string &serv) {

        string_view key(serv);
        auto iterator = serverAppChs.find(key);
        if (iterator != serverAppChs.end()) {
            auto i = iterator->second->addChannel(channelPtr);
            assert(i > 1);
            return i;
        } else {
            // 之前没有提供过相应的服务
            char *servName = static_cast<char *>(malloc(serv.size() + 1));
            memcpy(servName, serv.data(), serv.size());
            servName[serv.size()] = '\0';
            string_view x(servName, serv.size());
            serverAppChs.insert(make_pair(x, make_shared<ChannelHolder>(channelPtr)));
            LOG(INFO) << "Server app 注册服务成功:" << x << "当前" << serverAppChs.size() << "个服务注册";
            namingServer->registerService(x);
            return 1;
        }
    }

    void Server::removeServerAppChannel(ChannelPtr &channelPtr, const string &serv) {
        auto iterator = serverAppChs.find(serv);
        if (iterator != serverAppChs.end()) {
            auto i = iterator->second->removeChannel(channelPtr);
            if (i == 0) {
                const char *key = iterator->first.data();
                LOG(INFO) << "取消注册提供的服务:" << iterator->first << ",当前剩余"
                          << (serverAppChs.size() - 1) << "个服务";
                namingServer->unregisterService(serv);
                serverAppChs.erase(iterator);
                free((void *) key);
            }
        }
    }

    ChannelHolder *Server::getChannelByService(const string_view &serv) {
        auto iterator = serverAppChs.find(serv);
        if (iterator != serverAppChs.end()) {
            return iterator->second.get();
        }
        LOG(INFO) << "在" << serverAppChs.size() << "个中没找到ServerApp的channel:" << serv;
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