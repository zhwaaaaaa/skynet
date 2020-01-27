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

        int CurrentEvent(){
            return event;
        }

        Channel();

        virtual int Init();

        int MarkNonBlock() const;

        int getLocalAddr(EndPoint *out) const;

        virtual ~Channel();

        virtual int AddEventLoop(const EventDispatcher *dispatcher);

        friend class EventDispatcher;

    protected:
        virtual void OnEvent(int evt)=0;
    };

}

#endif //MESHER_CHANNEL_H
