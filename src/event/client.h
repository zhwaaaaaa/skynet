//
// Created by dear on 2020/2/5.
//

#ifndef SKYNET_CLIENT_H
#define SKYNET_CLIENT_H

#include "reactor.h"
#include "service_keeper.h"
#include <util/buffer.h>
#include <util/HASH_MAP_HPP.h>

namespace sn {
    using ServiceNamePtr = Segment<uint8_t> *;

    class Client : public Reactor {
    private:
        hash_map<string_view, shared_ptr<ServiceKeeper>> serviceMap;
    protected:

        void onLoopStart() override {
        }

        virtual void onLoopStop() override {
        }

    public:

        ServiceKeeper *getByService(ServiceNamePtr serv);

        void enableService(uint32_t channelId, ServiceNamePtr serv);

        void disableService(uint32_t channelId, ServiceNamePtr serv);

    };
}


#endif //SKYNET_CLIENT_H
