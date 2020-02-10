//
// Created by dear on 2020/2/5.
//

#include "service_keeper.h"
#include "reactor.h"

#include <utility>
#include <registry/naming_server.h>
#include <thread/thread.h>
#include <nevent/stream_channel.h>

namespace sn {


    ServiceKeeper::ServiceKeeper(const string_view &serv) : serv(serv) {}

    ServiceKeeper::~ServiceKeeper() {
        Thread::local<NamingServer>().unsubscribe(serv);
        for (const auto &pair: chMap) {
            pair.second->close();
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
                pair.second->close();
                chMap.erase(pair.first);
            }
        }

        // 新增的连接进来
        for (const EndPoint ep: eps) {
            if (chMap.find(ep) == chMap.end()) {
                // 不存在，新增的。
                auto pChannel = new TcpChannel<ClientReqHandler>;
                try {
//                    Thread::local<Reactor>().addLoopable(*pChannel);
                    pChannel->connectTo(ep);
                    chMap.insert(make_pair(ep, pChannel));
                } catch (const IoError &e) {
                    LOG(INFO) << e << " on connecting to " << ep;
                    delete pChannel;
                }
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
        return iterator->second;
    }


}


