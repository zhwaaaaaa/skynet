//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_TCP_LISTENER_H
#define SKYNET_TCP_LISTENER_H

#include <glog/logging.h>
#include "channel.h"
#include "io_error.h"
#include "stream_channel.h"

namespace sn {

    template<class Handler>
    class TcpListener : public Loopable {
    private:

        static void onConnectionArrived(uv_stream_t *server, int status) {
            if (status) {
                LOG(WARNING) << "error receive connection:" << uv_err_name(status);
                return;
            }

            auto tcpCh = new TcpChannel<Handler>;
            try {
                tcpCh->acceptFrom(server);
            } catch (IoError &e) {
                delete tcpCh;
                LOG(ERROR) << e;
            }

        }

    private:
        uv_tcp_t handle;
        EndPoint raddr;
        int backLog;


    public:
        TcpListener(const EndPoint &raddr, int backLog = 128) : raddr(raddr), backLog(backLog) {
        }

    public:
        void addToLoop(uv_loop_t *loop) {
            handle.data = this;
            uv_tcp_init(loop, &handle);

            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr = raddr.ip;
            serv_addr.sin_port = htons(static_cast<uint16_t>(raddr.port));

            int r;
            if ((r = uv_tcp_bind(&handle, (sockaddr *) &serv_addr, 0))) {
                LOG(FATAL) << "Error bind:" << uv_err_name(r);
                throw IoError(r, "Error bind");
            }
            if ((r = uv_listen((uv_stream_t *) &handle, backLog, onConnectionArrived))) {
                LOG(FATAL) << "Error listen:" << uv_err_name(r);
                throw IoError(r, "Error listen");
            }

            LOG(INFO) << "Listen at " << raddr;
        }

    };
}


#endif //SKYNET_TCP_LISTENER_H
