//
// Created by dear on 2020/2/13.
//

#ifndef SKYNET_SERVER_H
#define SKYNET_SERVER_H

#include "reactor.h"
#include "service_keeper.h"

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;
    using BodyDescPtr = Segment<uint32_t> *;
    struct ServiceDesc {
        ServiceNamePtr name;
        BodyDescPtr param;
        BodyDescPtr result;
    };


    class Server : public Reactor {
    private:
        hash_map<string_view, shared_ptr<ChannelHolder>> serverAppChs;
        hash_map<uint32_t, ChannelPtr> responseChs;
    public:
        void addServerAppChannel(ChannelPtr &channelPtr, vector<ServiceDesc> &sds);

        void removeServerAppChannel(ChannelPtr &channelPtr, vector<string> &sds);

        ChannelHolder *getChannelByService(const string_view &serv);

        Channel *getResponseChannel(uint32_t channelId);

        void addResponseChannel(ChannelPtr &respCh);

        void removeResponseChannel(uint32_t channelId);
    };
}


#endif //SKYNET_SERVER_H
