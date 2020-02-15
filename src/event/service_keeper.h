//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_SERVICE_KEEPER_H
#define SKYNET_SERVICE_KEEPER_H

#include <string>
#include <vector>
#include <nevent/channel.h>
#include <util/HASH_MAP_HPP.h>
#include "channel_keeper.h"

namespace sn {
    using namespace std;

    class ChannelGroup {
    public:
        /*virtual int addChannel(ChannelPtr &ch) = 0;

        virtual int removeChannel(ChannelPtr &ch) = 0;

        virtual int removeChannel(uint32_t channelId) = 0;*/

        [[nodiscard]] virtual Channel *nextChannel() = 0;

        [[nodiscard]] virtual int channelSize() const = 0;
    };


    class ServiceKeeper : public ChannelGroup {
    private:
        const string_view serv;
        hash_set<uint32_t> careCh;
        hash_map<EndPoint, shared_ptr<ChannelKeeper>> chMap;
        vector<EndPoint> endPoints;
        uint32_t nextIndex;

    public:
        explicit ServiceKeeper(const string_view &serv);

        ~ServiceKeeper();

        void servChanged(const vector<EndPoint> &eps);

        void care(uint32_t channelId) {
            careCh.insert(channelId);
        }

        bool noCare(uint32_t channelId) {
            auto iterator = careCh.find(channelId);
            if (iterator != careCh.end()) {
                careCh.erase(iterator);
                return true;
            }
            return false;
        }


        int count() {
            return careCh.size();
        }

        Channel *nextChannel() override;

        int channelSize() const override {
            return endPoints.size();
        }
    };

}


#endif //SKYNET_SERVICE_KEEPER_H
