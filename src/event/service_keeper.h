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
        static void onNamingServerNotify(const string &, const vector<EndPoint> &, bool hashNext, void *param);

    private:
        const string serv;
        hash_set<uint32_t> careCh;
        hash_map<EndPoint, Channel *> chMap;
        vector<EndPoint> currentEps;
        uint32_t lastIndex;

    public:
        explicit ServiceKeeper(const string &serv);

        ~ServiceKeeper();

        void servChanged(const vector<EndPoint> &eps);

        void care(uint32_t channelId);

        Channel *get();

        int count() {
            return careCh.size();
        }

    };

}


#endif //SKYNET_SERVICE_KEEPER_H
