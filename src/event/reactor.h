//
// Created by dear on 20-1-31.
//

#ifndef SKYNET_REACTOR_H
#define SKYNET_REACTOR_H

#include <thread/thread.h>
#include "event_dispatcher.h"
#include <queue>
#include <util/func_time.h>

namespace sn {


    class Reactor {
    protected:

        void callWakeUpFunc();


    private:
        mutex funcLock;
        queue<FuncWrap> funcQue;
    protected:
        Thread *thread;
        EventDispatcher *dispatcher;

    public:

        Reactor() : thread(nullptr), dispatcher(nullptr),
                    funcLock(), funcQue() {
        }


        void addFuncAndWakeup(ActionFunc func, void *param) {
            lock_guard g(funcLock);
            funcQue.push({param, func});
        }

        virtual void Stop()=0;
    };


    class IoThread : Thread {
    private:
        EventDispatcher *dispatcher;
    public:
        explicit IoThread(EventDispatcher *dispatcher) : dispatcher(dispatcher) {
            detach();
        }

    public:

        void run() final {


        }
    };

}


#endif //SKYNET_REACTOR_H
