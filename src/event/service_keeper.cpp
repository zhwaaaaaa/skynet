//
// Created by dear on 2020/2/5.
//

#include "service_keeper.h"
#include "client.h"

#include <utility>
#include <registry/naming_server.h>
#include <thread/thread.h>

namespace sn {

    ServiceKeeper::ServiceKeeper(const string_view &serv) : serv(serv), nextIndex(0) {}

    ServiceKeeper::~ServiceKeeper() {
        for (const auto &pair: chMap) {
            // 服务器
            pair.second->removeService(serv);
        }
    }

    void ServiceKeeper::servChanged(const std::vector<EndPoint> &eps) {

        vector<EndPoint> toRemove;
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
                toRemove.push_back(pair.first);
            }
        }

        for (const auto ep:toRemove) {
            auto iterator = chMap.find(ep);
            iterator->second->removeService(serv);
            chMap.erase(iterator);
        }

        // 新增的连接进来
        for (const EndPoint ep: eps) {
            if (chMap.find(ep) == chMap.end()) {
                auto y = Thread::local<Client>().getServiceChannel(ep);
                chMap.insert(make_pair(ep, y));
                y->addService(serv.data());
            }
        }

        endPoints = eps;
    }

    Channel *ServiceKeeper::nextChannel() {
        return chMap[endPoints[++nextIndex % endPoints.size()]]->channel();
    }

}


