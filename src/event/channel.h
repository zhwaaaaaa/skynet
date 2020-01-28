//
// Created by dear on 19-12-22.
//

#ifndef MESHER_CHANNEL_H
#define MESHER_CHANNEL_H


#include <zconf.h>
#include "event_dispatcher.h"
#include "endpoint.h"

namespace sn {

    class Channel {
    private:
        int event;
    protected:
        int fd;
        const EventDispatcher *dispatcher;
    public:

        int CurrentEvent() {
            return event;
        }

        Channel();

        virtual int Init()=0;

        virtual ~Channel();

        virtual int AddEventLoop(const EventDispatcher *dispatcher);

        virtual int OnEvent(int evt) final;

        friend class EventDispatcher;

    protected:

        virtual void doClose()=0;

        virtual int doRead()=0;

        virtual int doWrite()=0;

        bool markRead() {
            if (event & EVENT_READABLE == 0) {
                event |= EVENT_READABLE | EVENT_UPDATE;
                return true;
            }
            return false;
        }

        bool clearRead() {
            if (event & EVENT_READABLE) {
                event ^= EVENT_READABLE;
                event |= EVENT_UPDATE;
                return true;
            }
            return false;
        }

        bool markWrite() {
            if (event & EVENT_WRITABLE == 0) {
                event |= EVENT_WRITABLE | EVENT_UPDATE;
                return true;
            }
            return false;
        }

        bool clearWrite() {
            if (event & EVENT_WRITABLE) {
                event ^= EVENT_WRITABLE;
                event |= EVENT_UPDATE;
                return true;
            }
            return false;
        }
    };

}

#endif //MESHER_CHANNEL_H
