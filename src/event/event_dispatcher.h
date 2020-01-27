//
// Created by dear on 19-12-22.
//

#ifndef MESHER_EVENT_DISPATCHER_H
#define MESHER_EVENT_DISPATCHER_H

#include <sys/epoll.h>
#include <stdexcept>

#define MAX_EVENTS 100
#define EVENT_NONE 0       /* No events registered. */
#define EVENT_READABLE 1   /* Fire when descriptor is readable. */
#define EVENT_WRITABLE 2   /* Fire when descriptor is writable. */
#define EVENT_CLOSE 4
#define EVENT_UPDATE 8

namespace sn {
    class Channel;

    class EventDispatcher {
    private:
        int select_fd;
        epoll_event *events;
    public:
        explicit EventDispatcher(int maxEvt);

        int Select(int time);

        int AddChannelEvent(Channel *channel, int mask) const;

        int ModChannelEvent(Channel *channel, int mask) const;

        int DelChannelEvent(const Channel *channel) const;

        virtual ~EventDispatcher();

    };
}


#endif //MESHER_EVENT_DISPATCHER_H
