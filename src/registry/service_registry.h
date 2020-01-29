//
// Created by dear on 20-1-27.
//

#ifndef SKYNET_SERVICE_REGISTRY_H
#define SKYNET_SERVICE_REGISTRY_H

#include <event/channel.h>
#include <util/buffer.h>
#include "naming_server.h"
#include <string>
#include <map>

namespace sn {
    using namespace std;

    class ServiceRegistry {
    private:

        typedef map<EndPoint, Channel *> EndPointChannelMap;

        NamingServer *namingServer;
        EventDispatcher *dispatcher;
        hash_map<string, EndPointChannelMap *> connMap;
    public:
        explicit ServiceRegistry(NamingServer *namingServer, EventDispatcher *dispatcher)
                : namingServer(namingServer), dispatcher(dispatcher) {
        }

        virtual ~ServiceRegistry();

        /**
         * 像注册中心添加服务
         * @param segment server name
         * @return -1 error, 0 not found service. N(1-255) 当前连接数
         */
        virtual int addRequiredService(Segment<uint8_t> *segment);

    };

    ServiceRegistry *createServiceRegistry(EventDispatcher *dispatcher);

    ServiceRegistry *getServiceRegistry();

}


#endif //SKYNET_SERVICE_REGISTRY_H
