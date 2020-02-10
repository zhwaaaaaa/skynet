//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_SERVICE_KEEPER_H
#define SKYNET_SERVICE_KEEPER_H

#include <string>
#include <vector>
#include <nevent/channel.h>
#include <util/HASH_MAP_HPP.h>

namespace sn {
    using namespace std;

    class ServiceKeeper {


    private:
        const string_view serv;
        hash_set<uint32_t> careCh;
        hash_map<EndPoint, Channel *> chMap;
        vector<EndPoint> currentEps;
        uint32_t lastIndex;

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

        Channel *getChannel();

        int count() {
            return careCh.size();
        }

    };

}


#endif //SKYNET_SERVICE_KEEPER_H
