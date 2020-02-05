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
    using namespace std;

    void ServiceKeeper::onNamingServerNotify(const string &serv, const vector<EndPoint> &eps,
                                             bool hashNext, void *param) {
        ServiceKeeper *thisSk = static_cast<ServiceKeeper *>(param);
        if (!hashNext) {
            return;
        }
        thisSk->servChanged(eps);
    }

    ServiceKeeper::ServiceKeeper(const string &serv) : serv(serv) {
        Thread::local<NamingServer>().subscribe(serv, onNamingServerNotify, this);
    }

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
                auto pChannel = new TcpChannel<ClientTransferHandler>;
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

    void ServiceKeeper::care(uint32_t channelId) {
        careCh.insert(channelId);
    }

    Channel *ServiceKeeper::get() {
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


