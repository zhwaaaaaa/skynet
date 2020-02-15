//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_CLIENT_H
#define SKYNET_CLIENT_H

#include "reactor.h"
#include "service_keeper.h"
#include "channel_keeper.h"
#include <util/buffer.h>
#include <util/HASH_MAP_HPP.h>
#include <registry/naming_server.h>

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    class Client : public Reactor {
    private:
        static void onNamingServerNotify(const string_view &, const vector<EndPoint> &, bool hashNext, void *param);

    private:
        hash_map<string_view, shared_ptr<ServiceKeeper>> serviceMap;
        hash_map<EndPoint, shared_ptr<ChannelKeeper>> channelMap;
        hash_map<uint32_t, ChannelPtr> responseChs;
    protected:

        void onLoopStart() override {
            Thread::local<Client>(this);
            Thread::local<NamingServer>(new DemoNamingServer);
        }

        void onLoopStop() override {
            Thread::localRelease<Client>();
        }

    public:

        ServiceKeeper *getByService(ServiceNamePtr serv);

        void addServiceChannel(EndPoint endPoint);

        shared_ptr<ChannelKeeper> getServiceChannel(EndPoint endPoint);

        void removeServiceChannel(EndPoint endPoint);

        int enableService(uint32_t channelId, const string_view &key);

        int disableService(uint32_t channelId, const string_view &key);

        Channel *getResponseChannel(uint32_t channelId);

        void addResponseChannel(ChannelPtr &respCh);

        void removeResponseChannel(uint32_t channelId);

    };
}


#endif //SKYNET_CLIENT_H
