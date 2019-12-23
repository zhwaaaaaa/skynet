//
// Created by dear on 19-12-22.
//

#ifndef MESHER_EVENT_DISPATCHER_H
#define MESHER_EVENT_DISPATCHER_H

#include <sys/epoll.h>
#include <stdexcept>

#define Event_NONE 0       /* No events registered. */
#define Event_READABLE 1   /* Fire when descriptor is readable. */
#define Event_WRITABLE 2   /* Fire when descriptor is writable. */
#define Event_BARRIER 4    /* With WRITABLE, never fire the event if the
                           READABLE event already fired in the same event
                           loop iteration. Useful when you want to persist
                           things to disk before sending replies, and want
                           to do that in a group fashion. */

#define MAX_EVENTS 100

class Channel;

class EventDispatcher {
private:
    int select_fd;
    epoll_event *events;
public:
    EventDispatcher(int maxEvt);

    int Select(int time);

    int AddChannelEvent(Channel *channel, int mask) const;

    int ModChannelEvent(Channel *channel, int mask) const;

    int DelChannelEvent(const Channel *channel) const;

    virtual ~EventDispatcher();

};


#endif //MESHER_EVENT_DISPATCHER_H
