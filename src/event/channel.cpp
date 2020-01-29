//
// Created by dear on 19-12-22.
//

#include "channel.h"

namespace sn {
    Channel::Channel() : fd(-1),
                         dispatcher(nullptr),
                         event(EVENT_NONE) {
    }

    Channel::Channel(int fd) : fd(fd), dispatcher(nullptr), event(EVENT_NONE) {

    }

    Channel::~Channel() {
        if (dispatcher) {
            dispatcher->DelChannelEvent(this);
            dispatcher = nullptr;
        }
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

    int Channel::AddEventLoop(const EventDispatcher *dispatcher) {
        if (fd < 0) {
            return -2;
        }
        this->dispatcher = dispatcher;
        return dispatcher->AddChannelEvent(this, EVENT_READABLE);
    }

    int Channel::OnEvent(int mask) {
        if (mask & EVENT_CLOSE) {
            goto err;
        }

        if (mask & EVENT_READABLE) {
            if (doRead() != 0) {
                goto err;
            }
        }

        if (mask & EVENT_WRITABLE) {
            if (doWrite() != 0) {
                goto err;
            }
        }

        if (event & EVENT_UPDATE) {
            dispatcher->ModChannelEvent(this, event & EVENT_MASK);
        }
        return 0;
        err:
        dispatcher->DelChannelEvent(this);
        doClose();
        return -1;
    }

    int Channel::Init() {
        if (fd >= 0) {
            mark_non_block(fd);
            set_tcp_keep_alive(fd);
            set_tcp_no_delay(fd);
        }
        return 0;
    }


}

