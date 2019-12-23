//
// Created by dear on 19-12-22.
//

#ifndef MESHER_SOCKET_CHANNEL_H
#define MESHER_SOCKET_CHANNEL_H


#include "channel.h"
#include <string>

#define DEFAULT_BACKLOG 128

class SocketChannel : public Channel {
public:
    explicit SocketChannel(int port) : host("0.0.0.0"), port(port), backlog(DEFAULT_BACKLOG) {
    }

    int Init() override;

    explicit SocketChannel(const char *host, int port, int backlog = DEFAULT_BACKLOG) : host(host),
                                                                                        port(port),
                                                                                        backlog(backlog) {
    }

    virtual void OnEvent(int evt);

private:
    int port;
    std::string host;
    int backlog;
};


#endif //MESHER_SOCKET_CHANNEL_H
