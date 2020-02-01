//
// Created by dear on 19-12-22.
//

#ifndef MESHER_EVENT_DISPATCHER_H
#define MESHER_EVENT_DISPATCHER_H

#include <sys/epoll.h>
#include <stdexcept>
#include <thread/thread.h>
#include <map>
#include <util/func_time.h>

#define MAX_EVENTS 100
#define EVENT_NONE 0       /* No events registered. */
#define EVENT_READABLE 1   /* Fire when descriptor is readable. */
#define EVENT_WRITABLE 2   /* Fire when descriptor is writable. */
#define EVENT_CLOSE 4
#define EVENT_UPDATE 8
#define EVENT_MASK 7

namespace sn {
    class Channel;

    class EventChannel;

    using namespace std;

#define DISPATCHER_INIT 0
#define DISPATCHER_LOOP 1
#define DISPATCHER_CLOSE 2

    class EventDispatcher {
    private:

        friend class EventChannel;

        friend class Channel;

    private:
        int select_fd;
        epoll_event *events;
        multimap<long, FuncWrap> timerMap;
        EventChannel *eventChannel;
        int status;

        long invokeTimerAndGetNextTime();

        int Select(int time);

        int AddChannelEvent(Channel *channel, int fd, int mask = EVENT_READABLE) const;

        int ModChannelEvent(Channel *channel, int fd, int mask) const;

        int DelChannelEvent(int fd) const;

    public:
        explicit EventDispatcher(int maxEvt);

        void RunLoop();

        int addTimer(long timeout, ActionFunc func, void *param);

        /**
         * 除了这个函数以外，其他只允许本线程调用
         * @return
         */
        int wakeUp();

        void StopLoop();

        virtual ~EventDispatcher();

    };

}


#endif //MESHER_EVENT_DISPATCHER_H
