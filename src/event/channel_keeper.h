//
// Created by dear on 2020/2/10.
//

#ifndef SKYNET_CHANNEL_KEEPER_H
#define SKYNET_CHANNEL_KEEPER_H

#include <util/endpoint.h>
#include <string_view>
#include <nevent/channel.h>
#include <memory>

namespace sn {

    using std::string_view;
    using ChannelPtr = std::shared_ptr<Channel>;

    class ChannelKeeper {
    private:
        ChannelPtr channelPtr;
        hash_set<string_view> services;
    public:
        explicit ChannelKeeper(Channel *channel);

        ~ChannelKeeper();

        const hash_set<string_view> &getService() {
            return services;
        }

        void addService(const string_view &serv) {
            services.insert(serv);
        }

        void removeService(const string_view &serv) {
            services.erase(serv);
            if (services.empty()) {
                closeChannel();
            }
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

        ChannelPtr &channelPointer() {
            return channelPtr;
        }

        void closeChannel() {
            if (channelPtr.get()) {
                channelPtr->close();
            }
        }

    };

}


#endif //SKYNET_CHANNEL_KEEPER_H
