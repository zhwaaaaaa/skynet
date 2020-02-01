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
        unregisterDispatcher();
    }

    void Channel::unregisterDispatcher(bool closeFd) {
        if (fd >= 0) {
            if (dispatcher) {
                dispatcher->DelChannelEvent(fd);
                dispatcher = nullptr;
            }

            if (closeFd) {
                close(fd);
                fd = -1;
            }
        }

        if (dispatcher) {
            dispatcher = nullptr;
        }
    }

    int Channel::AddEventLoop(EventDispatcher *dispatcher) {
        if (fd < 0) {
            return -2;
        }
        this->dispatcher = dispatcher;
        return dispatcher->AddChannelEvent(this, fd);
    }

    int Channel::OnEvent(int mask) {
        if (mask & EVENT_CLOSE) {
            return -1;
        }

        if (mask & EVENT_READABLE) {
            if (doRead() != 0) {
                return -1;
            }
        }

        if (mask & EVENT_WRITABLE) {
            if (doWrite() != 0) {
                return -1;
            }
        }

        if (event & EVENT_UPDATE) {
            dispatcher->ModChannelEvent(this, fd, event & EVENT_MASK);
        }
        return 0;
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

