//
// Created by dear on 2020/2/13.
//

#ifndef SKYNET_SERVER_H
#define SKYNET_SERVER_H

#include "reactor.h"
#include "service_keeper.h"
#include <util/Convert.h>
#include <registry/naming_server.h>
#include <boost/unordered_map.hpp>

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    class ChannelHolder : public ChannelGroup {
    private:
        uint32_t index;
    protected:
        vector<ChannelPtr> chs;
    public:
        ChannelHolder();

        ~ChannelHolder();

        explicit ChannelHolder(ChannelPtr &first);

        int addChannel(ChannelPtr &ch);

        int removeChannel(ChannelPtr &ch);

        int removeChannel(uint32_t channelId);

        Channel *nextChannel() override;

        int channelSize() const override {
            return chs.size();
        }
    };

    class Server : public Reactor {
    private:
        boost::unordered_map<string_view, shared_ptr<ChannelHolder>> serverAppChs;
        boost::unordered_map<uint32_t, ChannelPtr> responseChs;
        NamingServer *namingServer;
    public:
        explicit Server(NamingServer &server);

    protected:
        void onLoopStart() override;

        void onLoopStop() override;


    public:
        int addServerAppChannel(ChannelPtr &ptr, const string &serv);

        void removeServerAppChannel(ChannelPtr &channelPtr, const string &serv);

        ChannelHolder *getChannelByService(const string_view &serv);

        Channel *getResponseChannel(uint32_t channelId);

        void addResponseChannel(ChannelPtr &respCh);

        void removeResponseChannel(uint32_t channelId);

    };
}


#endif //SKYNET_SERVER_H
