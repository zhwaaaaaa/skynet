//
// Created by dear on 20-1-31.
//

#include "reactor.h"

namespace sn {


    Reactor::Reactor() : thread(nullptr) {
        uv_loop_init(&loop);
        loop.data = this;
    }

    void Reactor::runLoop() {
        thread = Thread::current();
        Thread::local<Reactor>(this);
        onLoopStart();
        uv_run(&loop, UV_RUN_DEFAULT);
        onLoopStop();
        Thread::localRelease<Reactor>();
    }

    void Reactor::start(bool newThread) {
        if (!newThread) {
            runLoop();
        } else {
            auto *work = static_cast<uv_work_t *>(malloc(sizeof(uv_work_t)));
            work->data = this;
            uv_queue_work(&loop, work, runInThread, nullptr);
        }
    }


    void Reactor::stop() {
        if (!thread) {
            //not start
            return;
        }
        uv_async_t *async = static_cast<uv_async_t *>(malloc(sizeof(uv_async_t)));
        async->data = this;
        if (Thread::current() == thread) {
            notifyStop(async);
        } else {
            uv_async_init(&loop, async, notifyStop);
            uv_async_send(async);
        }
    }

    struct AsyncContext {
        uv_async_t uvHandle;
        EventFunc func;
        void *p;
        Reactor *reactor;

        AsyncContext(EventFunc func, void *p, Reactor *reactor) : func(func), p(p), reactor(reactor) {
            uvHandle.data = this;
        }
    };

    void Reactor::asyncEventExec(uv_async_t *handle) {
        AsyncContext *ctx = static_cast<AsyncContext *>(handle->data);
        ctx->func(ctx->p);
        delete ctx;
    }

    void Reactor::sendEvent(EventFunc func, void *p) {
        AsyncContext *ctx = new AsyncContext(func, p, this);
        uv_async_t *async = &ctx->uvHandle;
        uv_async_init(&loop, async, asyncEventExec);
        uv_async_send(async);
    }

    void Reactor::runInThread(uv_work_t *req) {
        Reactor *reactor = static_cast<Reactor *>(req->data);
        free(req);
        reactor->runLoop();
    }

    void Reactor::notifyStop(uv_async_t *handle) {
        Reactor *reactor = static_cast<Reactor *>(handle->data);
        free(handle);
        uv_stop(&reactor->loop);
    }


}


