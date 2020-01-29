//
// Created by dear on 20-1-27.
//

#include <glog/logging.h>
#include "service_registry.h"
#include <event/tcp_channel.h>

//#define SERVICE_MAX_CONN 10
namespace sn {
    using namespace std;

    int ServiceRegistry::addRequiredService(Segment<uint8_t> *segment) {
        string serv = string(segment->buf, segment->len);
        const std::vector<EndPoint> vector = namingServer->find(serv);
        int i = vector.size();
        if (i == 0) {
            return 0;
        }

        EndPointChannelMap *map = nullptr;
        for (const auto &np:vector) {
            int fd = tcp_connect(np, nullptr);
            if (fd <= 0) {
                LOG(WARNING) << "Error Connect To " << np;
                i--;
                continue;
            }
            if (!map) {
                map = new EndPointChannelMap;
            }

            Channel *ch = new TcpChannel(fd, np);
            ch->Init();
            map->insert(make_pair(np, ch));
        }
        if (map) {
            this->connMap.insert(make_pair(serv, map));
            namingServer->subscribe(serv, [](const auto &s, const auto &nps) {
                return 0;
            });
            return map->size();
        }


        return i;
    }

    ServiceRegistry::~ServiceRegistry() {
        for (const auto &pair:  connMap) {
            namingServer->unsubscribe(pair.first);

            EndPointChannelMap &ip_ch = *pair.second;
            for (const auto &pnpc:ip_ch) {
                LOG(INFO) << "Closing " << pair.first.data() << "->" << pnpc.first;
                delete pnpc.second;
                ip_ch.erase(pnpc.first);
            }
            LOG(INFO) << "Service:" << pair.first.data() << " Removed";
        }

    }

    static ServiceRegistry *serviceRegistry;

    ServiceRegistry *createServiceRegistry(EventDispatcher *dispatcher) {
        if (serviceRegistry == nullptr) {
            serviceRegistry = new ServiceRegistry(new DemoNamingServer, dispatcher);
        }
        return serviceRegistry;
    }

    ServiceRegistry *getServiceRegistry() {
        return serviceRegistry;
    }

}
