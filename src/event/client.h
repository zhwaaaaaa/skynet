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

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    class Client : public Reactor {
    private:
        static void onNamingServerNotify(const string_view &, const vector<EndPoint> &, bool hashNext, void *param);

    private:
        hash_map<string_view, shared_ptr<ServiceKeeper>> serviceMap;
        hash_map<EndPoint, shared_ptr<ChannelKeeper>> channelMap;
    protected:

        void onLoopStart() override {
        }

        virtual void onLoopStop() override {
        }

    public:

        ServiceKeeper *getByService(ServiceNamePtr serv);

        void addServiceChannel(const EndPoint endPoint);

        shared_ptr<ChannelKeeper> getServiceChannel(const EndPoint endPoint);

        void removeServiceChannel(const EndPoint endPoint);

        void enableService(uint32_t channelId, ServiceNamePtr serv);

        void disableService(uint32_t channelId, ServiceNamePtr serv);

    };
}


#endif //SKYNET_CLIENT_H
