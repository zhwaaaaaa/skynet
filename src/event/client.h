//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_CLIENT_H
#define SKYNET_CLIENT_H

#include "reactor.h"

namespace sn {


    class Client : public Reactor {
    protected:

        void onLoopStart() override {

        }

        virtual void onLoopStop() override {

        }

    public:

        Channel *getByService(const string &serv);

        void enableService(uint32_t channelId, const string &serv);

        void disableService(uint32_t channelId, const string &serv);

    };
}


#endif //SKYNET_CLIENT_H
