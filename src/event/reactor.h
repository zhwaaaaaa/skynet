//
// Created by dear on 20-1-31.
//

#ifndef SKYNET_REACTOR_H
#define SKYNET_REACTOR_H

#include <thread/thread.h>
#include <queue>
#include <util/func_time.h>
#include <uv.h>
#include <nevent/channel.h>
#include "channel_keeper.h"
#include <mutex>
#include <condition_variable>


namespace sn {

    typedef void (*EventFunc)(void *p);

    class Reactor {

    private:

        static void runInThread(uv_work_t *req);

        static void notifyStop(uv_async_t *handle);

        static void asyncEventExec(uv_async_t *handle);

        static void onSignalReceive(uv_signal_t *handle, int signum);

        static void onSignalReceiveOnce(uv_signal_t *handle, int signum);

        void runLoop();

    private:
        std::mutex stopLock;
        std::condition_variable condVar;
        bool stopped = false;

    protected:
        uv_loop_t loop;
        Thread *thread;

        virtual void onLoopStart() {

        }

        virtual void onLoopStop() {

        }

    public:
        Reactor();

        virtual ~Reactor();

        void addLoopable(Loopable &loopable) {
            loopable.addToLoop(&loop);
        }

        void listenSignal(EventFunc func, void *p, int signum, bool handleOnce = false);

        void sendEvent(EventFunc func, void *p);

        void start(bool newThread = false);

        void stop();

        void waitingStop();

    };

}


#endif //SKYNET_REACTOR_H
