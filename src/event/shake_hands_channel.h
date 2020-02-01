//
// Created by dear on 20-1-29.
//

#ifndef SKYNET_SHAKE_HANDS_CHANNEL_H
#define SKYNET_SHAKE_HANDS_CHANNEL_H


#include <registry/service_registry.h>
#include "conn_channel.h"

#define CONN_CLIENT 0
#define CONN_SERVER 0xFF

namespace sn {
    class ShakeHandsChannel : public ConnChannel {
    private:
        int packageLen = -1;
        int serviceSize = -1;
        int connType = -1;
        int status = 0;// 0 reading head ; 1 write body ;2 complete ; -1 close
        Buffer *resp = nullptr;
        ServiceRegistry *serviceRegistry;
    public:
        explicit ShakeHandsChannel(int fd, ServiceRegistry *serviceRegistry);

    protected:
        int tryDecodeBuf() override;

    private:
        int readServicesAndConnect();

        Action writeData(int status, ConnChannel *ch, void *param);

        int doWriteToFd();
    };

}


#endif //SKYNET_SHAKE_HANDS_CHANNEL_H
