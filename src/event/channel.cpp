//
// Created by dear on 19-12-22.
//

#include "channel.h"
#include <fcntl.h>

namespace sn {
    int markNonBlock(int fd) {
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

    Channel::Channel() : fd(-1), dispatcher(nullptr), event(EVENT_NONE) {

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

    void Channel::OnEvent(int mask) {
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
        return;
        err:
        dispatcher->DelChannelEvent(this);
        doClose();
    }
}

