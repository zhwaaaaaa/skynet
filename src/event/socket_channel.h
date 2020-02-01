//
// Created by dear on 19-12-22.
//

#ifndef MESHER_SOCKET_CHANNEL_H
#define MESHER_SOCKET_CHANNEL_H


#include "channel.h"
#include <string>

#define DEFAULT_BACKLOG 128

namespace sn {


    class SocketChannel : public Channel {
    public:
        explicit SocketChannel(EndPoint listenAddr,
                               int fd) : Channel(fd), listenAddr(listenAddr) {
        }

    protected:

        int doRead() override;

    private:
        EndPoint listenAddr;

    public:
        EndPoint ListenAddr() {
            return listenAddr;
        }
    };
}


#endif //MESHER_SOCKET_CHANNEL_H
