//
// Created by dear on 2020/2/5.
//

#include "service_keeper.h"
#include "reactor.h"

#include <utility>
#include <registry/naming_server.h>
#include <thread/thread.h>
#include <cassert>

namespace sn {


    ServiceKeeper::ServiceKeeper(const string_view &serv) : serv(serv), lastIndex(0) {}

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
                // 服务器
                pair.second->removeService(serv);
                chMap.erase(pair.first);
            }
        }

        // 新增的连接进来
        for (const EndPoint ep: eps) {
            if (chMap.find(ep) == chMap.end()) {
                chMap.insert(make_pair(ep, Thread::local<Reactor>().getServiceChannel(ep)));
            }
        }

        currentEps.clear();
        currentEps.insert(currentEps.end(), eps.begin(), eps.end());
    }


    Channel *ServiceKeeper::getChannel() {
        auto len = currentEps.size();
        if (!len) {
            // no connected channel
            return nullptr;
        }
        auto iterator = chMap.find(currentEps[lastIndex++ % len]);
        assert(iterator != chMap.end());
        return iterator->second->channel();
    }


}


