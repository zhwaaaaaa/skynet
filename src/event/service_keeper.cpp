//
// Created by dear on 2020/2/5.
//

#include "service_keeper.h"
#include "reactor.h"
#include "client.h"

#include <utility>
#include <registry/naming_server.h>
#include <thread/thread.h>

namespace sn {
    ChannelHolder::ChannelHolder() : index(0) {}

    ChannelHolder::ChannelHolder(ChannelPtr &first) : index(0) {
        chs.push_back(first);
    }

    int ChannelHolder::addChannel(ChannelPtr &ch) {

        for (const ChannelPtr &c:chs) {
            if (c->channelId() == ch->channelId()) {
                return chs.size();
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

    Channel *ChannelHolder::getChannel() {
        return chs[++index % chs.size()].get();
    }

    int ChannelHolder::removeChannel(uint32_t channelId) {
        auto iterator = remove_if(chs.begin(), chs.end(), [&](ChannelPtr &c) {
            return c->channelId() == channelId;
        });

        chs.erase(iterator);
        return chs.size();
    }


    ServiceKeeper::ServiceKeeper(const string_view &serv) : serv(serv) {}

    ServiceKeeper::~ServiceKeeper() {
        for (const auto &pair: chMap) {
            // 服务器
            pair.second->removeService(serv);
            chMap.erase(pair.first);
        }
    }

    void ServiceKeeper::servChanged(const std::vector<EndPoint> &eps) {

        // 不存在的关闭掉
        for (const auto &pair: chMap) {
            bool exits = false;
            for (const EndPoint ep: eps) {
                if (ep == pair.first) {
                    exits = true;
                    break;
                }
            }
            if (!exits) {
                auto channel = pair.second->channel();
                if (channel) {
                    removeChannel(channel->channelId());
                }
                // 服务器
                pair.second->removeService(serv);
                chMap.erase(pair.first);
            }
        }

        // 新增的连接进来
        for (const EndPoint ep: eps) {
            if (chMap.find(ep) == chMap.end()) {
                auto y = Thread::local<Client>().getServiceChannel(ep);
                chMap.insert(make_pair(ep, y));
                addChannel(y->channelPointer());
            }
        }
    }


}


