//
// Created by dear on 19-12-22.
//

#ifndef MESHER_CONN_CHANNEL_H
#define MESHER_CONN_CHANNEL_H

#include "channel.h"
#include <string>

class ConnChannel : public Channel {
private:
    std::string host;
    int port;
public:
    explicit ConnChannel(int fd, const char *host, int port) : Channel(), host(host), port(port) {
        this->fd = fd;
    }

    int Port() const {
        return port;
    }

    const std::string &Host() {
        return host;
    }

    virtual int Init() override;

    virtual void OnEvent(int mask) override;
};


#endif //MESHER_CONN_CHANNEL_H
