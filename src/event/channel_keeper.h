//
// Created by dear on 2020/2/10.
//

#ifndef SKYNET_CHANNEL_KEEPER_H
#define SKYNET_CHANNEL_KEEPER_H

#include <util/endpoint.h>
#include <string_view>
#include <nevent/channel_handler.h>

namespace sn {

    using std::string_view;

    class ChannelKeeper {
    private:
        ChannelPtr channelPtr;
        hash_set<string_view> services;
    public:
        ChannelKeeper(Channel *channel) : channelPtr(channel) {}

        const hash_set<string_view> &getService() {
            return services;
        }

        void resetChannel(ChannelPtr &ch) {
            channelPtr = ch;
        }

        int serviceCount() {
            return services.size();
        }

        Channel *channel() {
            return channelPtr.get();
        }

    };

}


#endif //SKYNET_CHANNEL_KEEPER_H
