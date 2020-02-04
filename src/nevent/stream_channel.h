//
// Created by dear on 2020/2/4.
//

#ifndef SKYNET_STREAM_CHANNEL_H
#define SKYNET_STREAM_CHANNEL_H

#include <glog/logging.h>
#include <cassert>
#include "channel.h"
#include "io_error.h"
#include "channel_handler.h"

namespace sn {

#if defined(_WIN32)
    static thread_local uint32_t _id=0;
#else
    static thread_local uint8_t _id_mask;
#endif

    template<class Type>
    class WritableChannel : public Channel {
    private:
#if defined(_WIN32)
        uint32_t id=0;
#else
        uint8_t id_high;
#endif
    protected:
        Type handle;
    public:
        WritableChannel() {
#if defined(_WIN32)
            id=_id++;
#else
            id_high = _id_mask++;
#endif
        }

        uint32_t channelId() override {
#if defined(_WIN32)
            return id;
#else
            return (id_high << 24) | handle.io_watcher.fd;
#endif
        }

        virtual void writeMsg(SegmentRef *ref) {
        }

        virtual void startWrite() {
        }

        void close() override {
            uv_close((uv_handle_t *) &handle, ChannelHandler::onChannelClosed);
        }


    };

    template<class Handler>
    class TcpChannel : public WritableChannel<uv_tcp_t> {
    private:
        EndPoint raddr;

        static void onConnected(uv_connect_t *req, int status) {
            if (!status) {
                free(req);
                return;
            }
            TcpChannel<Handler> *ch = req->data;
            auto pHandler = new Handler(ch);
            uv_stream_t *stream = req->handle;
            stream->data = pHandler;
            uv_read_start(stream, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
        }

    public:

        EndPoint remoteAddr() {
            return raddr;
        }

        void addToLoop(uv_loop_t *loop) override {
            uv_tcp_init(loop, (uv_tcp_t *) &handle);
        }

        void connectTo(EndPoint endPoint) {
            raddr = endPoint;

            uv_connect_t *conType = static_cast<uv_connect_t *>(malloc(sizeof(uv_connect_t)));
            conType->data = this;
            struct sockaddr_in serv_addr;
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr = endPoint.ip;
            serv_addr.sin_port = htons(static_cast<uint16_t>(endPoint.port));
            int r;
            if ((r = uv_tcp_connect(conType, &handle, (sockaddr *) &serv_addr, TcpChannel<Handler>::onConnected))) {
                throw IoError(r, "connection");
            }
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

            assert(storage.ss_family == AF_INET);
            sockaddr_in *in = reinterpret_cast<sockaddr_in *>(&storage);
            raddr = EndPoint(*in);
            LOG(INFO) << "Accept connection from " << raddr;
            handle.data = new Handler(this);
            uv_read_start(client, ChannelHandler::onMemoryAlloc, ChannelHandler::onMessageArrived);
        };

    };
}


#endif //SKYNET_STREAM_CHANNEL_H
