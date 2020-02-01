//
// Created by dear on 20-1-31.
//

#ifndef SKYNET_SERVER_H
#define SKYNET_SERVER_H

#include <util/endpoint.h>
#include <thread/thread.h>
#include "event_dispatcher.h"
#include "socket_channel.h"
#include "reactor.h"

namespace sn {
    class Server;

    static Server *server;

    class Server : public Reactor {

    public:
        static Server &getInstance();

    private:
        SocketChannel *inChannel;
        EndPoint enterAddr;

    public:
        explicit Server(EndPoint inAddr);

        ~Server();

        EndPoint EnterAddr() {
            return enterAddr;
        }


        int AddOutFd(int fd);

        int Start();

        void Stop() override;
    };

}


#endif //SKYNET_SERVER_H
