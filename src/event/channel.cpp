//
// Created by dear on 19-12-22.
//

#include "channel.h"
#include <fcntl.h>

namespace sn {

    Channel::Channel() : fd(-1), dispatcher(nullptr), event(EVENT_NONE) {

    }

    int Channel::Init() {
        if (this->fd < 0) {
            return -1;
        }
        return 0;
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

    int Channel::MarkNonBlock() const {
        int opts;
        opts = fcntl(fd, F_GETFL);
        if (opts < 0) {
            return -1;
        }
        opts = opts | O_NONBLOCK;
        if (fcntl(fd, F_SETFL, opts) < 0) {
            return -1;
        }
        return 0;
    }

    int Channel::getLocalAddr(EndPoint *out) const {
        return get_local_side(fd, out);
    }
}

