//
// Created by dear on 19-12-22.
//

#include "event_dispatcher.h"
#include <iostream>
#include <unistd.h>
#include "channel.h"

namespace sn {

    EventDispatcher::EventDispatcher(int maxEvt) {
        select_fd = epoll_create1(0);
        if (select_fd == -1) {
            perror("epoll_create1");
            throw std::runtime_error("epoll_create return -1 ");
        }
        events = static_cast<epoll_event *>(malloc(maxEvt * sizeof(epoll_event)));
    }


    int EventDispatcher::Select(int time) {
        int nfds;
        for (;;) {
            nfds = epoll_wait(select_fd, events, MAX_EVENTS, time);
            if (nfds == -1) {
                return -1;
            }
            for (int i = 0; i < nfds; ++i) {
                epoll_event &e = events[i];
                Channel *ptr = static_cast<Channel *>(e.data.ptr);
                int mask = 0;
                if (e.events & EPOLLIN) mask |= EVENT_READABLE; //读
                if (e.events & EPOLLOUT) mask |= EVENT_WRITABLE; //写
                if (e.events & EPOLLERR) mask |= EVENT_WRITABLE; //出错
                if (e.events & EPOLLHUP) mask |= EVENT_CLOSE; //关闭事件
                ptr->OnEvent(mask);
            }
        }

    }

    EventDispatcher::~EventDispatcher() {
        if (select_fd > 0) {
            close(select_fd);
        }
        free(events);
    }

    int EventDispatcher::AddChannelEvent(Channel *channel, int mask) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        ee.events = 0;
        if (mask & EVENT_READABLE) ee.events |= EPOLLIN | EPOLLET;
        if (mask & EVENT_WRITABLE) ee.events |= EPOLLOUT;
        ee.data.ptr = channel;
        if (epoll_ctl(select_fd, EPOLL_CTL_ADD, channel->fd, &ee) == -1) {
            return -1;
        }
        return 0;
    }

    int EventDispatcher::ModChannelEvent(Channel *channel, int mask) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        ee.events = 0;
        if (mask & EVENT_READABLE) ee.events |= EPOLLIN;
        if (mask & EVENT_WRITABLE) ee.events |= EPOLLOUT;
        int fd = channel->fd;
        ee.data.ptr = channel;
        if (epoll_ctl(select_fd, EPOLL_CTL_MOD, fd, &ee) == -1) {
            return -1;
        }
        return 0;
    }


    int EventDispatcher::DelChannelEvent(const Channel *channel) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        return epoll_ctl(select_fd, EPOLL_CTL_DEL, channel->fd, &ee);
    }
}
