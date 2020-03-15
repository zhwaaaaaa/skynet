//
// Created by dear on 2020/3/12.
//

#ifndef SKYNET_TCPCHANNEL_H
#define SKYNET_TCPCHANNEL_H

#include "stream_channel.h"
#include "io_error.h"
#include <uv.h>

namespace sn {

    template<class Handler>
    class TcpChannel : public WritableChannel<uv_tcp_t> {
    private:
        EndPoint raddr;

        static void timeoutReconnect(uv_timer_t *handle) {

            TcpChannel<Handler> *ch = static_cast<TcpChannel<Handler> *>(handle->data);
            free(handle);
            try {
                ch->connectTo(ch->raddr);
            } catch (const IoError &e) {
                LOG(ERROR) << "timeoutReconnect " << e;
                delete ch;
            }

        }

        static void onConnected(uv_connect_t *req, int status) {
            TcpChannel<Handler> *ch = static_cast<TcpChannel<Handler> *>(req->data);
            if (!status) {
                ChannelPtr ptr = make_shared<TcpChannel<Handler>>();
                auto pHandler = new Handler(ptr);
                uv_stream_t *stream = req->handle;
                stream->data = pHandler;
                uv_read_start(stream, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
            } else {
                LOG(ERROR) << "Connect error retry" << uv_strerror(status);
                uv_timer_t *timer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
                timer->data = ch;
                uv_timer_init(req->handle->loop, timer);
                uv_timer_start(timer, timeoutReconnect, 3000, 0);
            }
            free(req);
        }

    public:

        ~TcpChannel() override {
            LOG(INFO) << "~TcpChannel() " << raddr;
        }

        TcpChannel() {
            LOG(INFO) << "TcpChannel()" << raddr;
        }

        EndPoint remoteAddr() {
            return raddr;
        }

        void addToLoop(uv_loop_t *loop) override {
            uv_tcp_init(loop, (uv_tcp_t *) &handle);
        }

        /**
         * 如果这里抛出异常，回调函数一定不会执行。
         * 如果没有抛出异常。则会无限次重连
         * @param endPoint 远程
         */
        void connectTo(EndPoint endPoint) {
            raddr = endPoint;

            if (!handle.loop) {
                // lib_uv 每一个结构必须有一个loop
                addToLoop(uv_default_loop());
            }

            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr = endPoint.ip;
            serv_addr.sin_port = htons(static_cast<uint16_t>(endPoint.port));
            int r;

            struct sockaddr_in client_addr;
            if ((r = uv_ip4_addr("0.0.0.0", 0, &client_addr))) {
                throw IoError(r, "Init local addr");
            }
            if ((r = uv_tcp_bind(&handle, (sockaddr *) &client_addr, 0))) {
                throw IoError(r, "Bind");
            }

            uv_connect_t *conType = static_cast<uv_connect_t *>(malloc(sizeof(uv_connect_t)));
            conType->data = this;
            if ((r = uv_tcp_connect(conType, &handle, (sockaddr *) &serv_addr, onConnected))) {
                throw IoError(r, "Connection");
            }
            uv_tcp_keepalive(&handle, 1, 3600);
            uv_tcp_nodelay(&handle, 1);
        }

        void acceptFrom(uv_stream_t *server) {
            addToLoop(server->loop);
            int r;
            uv_stream_t *client = (uv_stream_t *) &handle;
            if ((r = uv_accept(server, client))) {
                throw IoError(r, "acceptFrom");
            }

            sockaddr_storage storage = {0};
            int len = sizeof(storage);

            if ((r = uv_tcp_getpeername(&handle, (sockaddr *) &storage, &len))) {
                throw IoError(r, "acceptFrom");
            }

            CHECK(storage.ss_family == AF_INET);
            sockaddr_in *in = reinterpret_cast<sockaddr_in *>(&storage);
            raddr = EndPoint(*in);
            LOG(INFO) << "Accept connection from " << raddr;
            ChannelPtr ptr = shared_ptr<TcpChannel<Handler>>(this);
            handle.data = new Handler(ptr);
            uv_tcp_keepalive(&handle, 1, 3600);
            uv_tcp_nodelay(&handle, 0);
            uv_read_start(client, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
        };

    };

}


#endif //SKYNET_TCPCHANNEL_H
