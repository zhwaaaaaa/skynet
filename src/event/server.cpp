//
// Created by dear on 20-1-31.
//

#include "server.h"

namespace sn {


    Server &Server::getInstance() {
        return *server;
    }

    Server::Server(EndPoint inAddr) : Reactor() {
        if (server) {
            throw runtime_error("Server can be create Once");
        }

        int fd = tcp_listen(inAddr);
        if (fd < 0) {
            LOG(ERROR) << "Listen " << inAddr << " Error in Server:" << strerror(errno);
            throw runtime_error("Listen Error in Server:");
        }
        inChannel = new SocketChannel(inAddr, fd);
        inChannel->Init();
        server = this;
    }

    Server::~Server() {
        if (dispatcher != nullptr) {
        }

        if (thread != nullptr) {
        }

    }

    int Server::AddOutFd(int fd) {
        return 0;
    }

    int Server::Start() {

        return 0;
    }

    void Server::Stop() {
    }


}


