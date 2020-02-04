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

namespace sn {


    class Reactor {

    private:

        static void runInThread(uv_work_t *req);

        static void notifyStop(uv_async_t *handle);

        void runLoop();

    protected:
        uv_loop_t loop;
        Thread *thread;

        virtual void onLoopStart() {

        }

        virtual void onLoopStop() {

        }

    public:
        Reactor();

        void addLoopable(Loopable &loopable) {
            loopable.addToLoop(&loop);
        }

        void start(bool newThread = false);

        void stop();

    };

}


#endif //SKYNET_REACTOR_H
