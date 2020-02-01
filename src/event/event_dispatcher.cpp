//
// Created by dear on 19-12-22.
//

#include "event_dispatcher.h"
#include <iostream>
#include "channel.h"
#include <sys/eventfd.h>
#include <util/func_time.h>

namespace sn {

    class EventChannel : public Channel {
    public:
        explicit EventChannel(int fd) : Channel(fd) {
        }

    protected:

        virtual int doRead() {
            uint64_t rVal;
            ssize_t readed = read(fd, &rVal, sizeof(uint64_t));
            if (readed == -1) {
                LOG(ERROR) << "ERROR read" << endl;
                return -1;
            }
        }

    public:
        int notifyEpollWakeup() {
            uint64_t val = 1;
            const auto i = write(fd, &val, sizeof(val));
            if (i == -1 && errno != EAGAIN) {
                return -1;
            }
            return 0;
        }
    };

    EventDispatcher::EventDispatcher(int maxEvt) : eventChannel(nullptr) {
        status = DISPATCHER_INIT;
        select_fd = epoll_create1(0);
        if (select_fd == -1) {
            LOG(FATAL) << "epoll_create return -1:" << strerror(errno);
            throw std::runtime_error("epoll_create return -1 ");
        }
        events = static_cast<epoll_event *>(malloc(maxEvt * sizeof(epoll_event)));
    }


    int EventDispatcher::Select(int time) {
        int nfds = epoll_wait(select_fd, events, MAX_EVENTS, time);
        if (nfds == -1) {
            // 系统调用被信号打断
            if (errno != EINTR) {
                LOG(ERROR) << "ERROR epoll_wait:" << strerror(errno);
                return -1;
            }
            return 0;
        }
        for (int i = 0; i < nfds; ++i) {
            epoll_event &e = events[i];
            Channel *ptr = static_cast<Channel *>(e.data.ptr);
            int mask = 0;
            if (e.events & EPOLLIN) mask |= EVENT_READABLE; //读
            if (e.events & EPOLLOUT) mask |= EVENT_WRITABLE; //写
            if (e.events & EPOLLERR) mask |= EVENT_WRITABLE; //出错
            if (e.events & EPOLLHUP) mask |= EVENT_CLOSE; //关闭事件
            if (ptr->OnEvent(mask) != 0) {
                delete ptr;
            }
        }
    }

    EventDispatcher::~EventDispatcher() {
        if (select_fd > 0) {
            close(select_fd);
        }
        free(events);
    }

    int EventDispatcher::AddChannelEvent(Channel *channel, int fd, int mask) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        ee.events = EPOLLHUP;
        if (mask & EVENT_READABLE) ee.events |= EPOLLIN | EPOLLET;
        if (mask & EVENT_WRITABLE) ee.events |= EPOLLOUT;
        ee.data.ptr = channel;
        if (epoll_ctl(select_fd, EPOLL_CTL_ADD, fd, &ee) == -1) {
            return -1;
        }
        return 0;
    }

    int EventDispatcher::ModChannelEvent(Channel *channel, int fd, int mask) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        ee.events = EPOLLHUP;
        if (mask & EVENT_READABLE) ee.events |= EPOLLIN | EPOLLET;
        if (mask & EVENT_WRITABLE) ee.events |= EPOLLOUT;
        ee.data.ptr = channel;
        if (epoll_ctl(select_fd, EPOLL_CTL_MOD, fd, &ee) == -1) {
            return -1;
        }
        return 0;
    }


    int EventDispatcher::DelChannelEvent(int fd) const {
        epoll_event ee = {0}; /* avoid valgrind warning */
        return epoll_ctl(select_fd, EPOLL_CTL_DEL, fd, &ee);
    }

    int EventDispatcher::addTimer(long timeout, ActionFunc func, void *param) {
        FuncWrap wrap{param, func};
        timerMap.insert(make_pair(currentTime() + timeout, wrap));
        return 0;
    }

    int EventDispatcher::wakeUp() {
        return eventChannel->notifyEpollWakeup();
    }

    long EventDispatcher::invokeTimerAndGetNextTime() {
        long nextTime = -1;
        const auto current = currentTime();
        for (const auto &pair:timerMap) {
            auto key = pair.first;
            if (key <= current) {
                const FuncWrap &wrap = pair.second;
                wrap.func(wrap.param);
                timerMap.erase(key);
            } else {
                nextTime = key;
                break;
            }
        }
    }

    void EventDispatcher::RunLoop() {
        if (status != DISPATCHER_INIT) {
            LOG(ERROR) << "[bug]event dispatcher status not init" << status;
            throw runtime_error("[bug]event dispatcher status not init");
        }

        status = DISPATCHER_LOOP;
        int eventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (eventFd == -1) {
            LOG(FATAL) << "eventFd return -1:" << strerror(errno);
            return;
        }
        eventChannel = new EventChannel(eventFd);
        if (eventChannel->AddEventLoop(this) != 0) {
            delete eventChannel;
            eventChannel = nullptr;
            LOG(FATAL) << "AddChannelEvent return -1:" << strerror(errno);
            return;
        }

        for (; status == DISPATCHER_LOOP;) {
            long nextWakeTime = invokeTimerAndGetNextTime();
            int timeout;
            if (nextWakeTime == -1) {
                timeout = -1;
            } else {
                timeout = static_cast<int>(nextWakeTime - currentTime());
                if (timeout < 0) {
                    timeout = 0;
                }
            }
            if (Select(timeout) == -1) {
                status = DISPATCHER_CLOSE;
                break;
            }
        }

        delete eventChannel;
    }

    void EventDispatcher::StopLoop() {
        status = DISPATCHER_CLOSE;
    }

}
