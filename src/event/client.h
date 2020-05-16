//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_CLIENT_H
#define SKYNET_CLIENT_H

#include "reactor.h"
#include "service_keeper.h"
#include "channel_keeper.h"
#include <registry/naming_server.h>
#include <util/Convert.h>
#include <boost/unordered_map.hpp>

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    class Client : public Reactor {
    private:
        static void onNamingServerNotify(const string_view &serv, const vector<string> &eps, void *param);

    private:
        boost::unordered_map<string_view, shared_ptr<ServiceKeeper>> serviceMap;
        boost::unordered_map<EndPoint, shared_ptr<ChannelKeeper>> channelMap;
        boost::unordered_map<uint32_t, ChannelPtr> responseChs;
        NamingServer *namingServer;
    protected:
        void onLoopStart() override;

        void onLoopStop() override;

    public:
        explicit Client(NamingServer &namingServer);

        ServiceKeeper *getByService(ServiceNamePtr serv);

        ServiceKeeper *getByService(const string_view &serv);

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
