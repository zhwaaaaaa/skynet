//
// Created by dear on 20-1-30.
//
#include <glog/logging.h>
#include "thread.h"

namespace sn {

    Thread::Thread() : id(0), exitCallBack(), callBackLock(), status(INIT) {
        int r = pthread_attr_init(&attr);
        if (r != 0) {
            LOG(FATAL) << "初始化属性失败";
        }
    }

    void Thread::join() const {
        pthread_join(id, nullptr);
    }

    Thread::ThreadId Thread::getId() const {
        return id;
    }

    void Thread::detach() {
        if (id) {
            if (pthread_detach(id)) {
                LOG(INFO) << "[BUG]ERROR thread detach";
            }
        } else {
            //https://www.cnblogs.com/jacklikedogs/p/4030048.html
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        }

    }

    void *Thread::threadRunFunc(void *ptr) {
        auto *thread = static_cast<Thread *>(ptr);
        thread->status = LIVED;
        localPtr<Thread>().reset(thread);
        thread->run();
        thread->status = EXITED;
        thread->destroyAndInvokeCallback();
        // run 函数还没执行完
    }

    void Thread::start() {
        if (id != 0) {
            throw runtime_error("[BUG] thread started");
        }
        int r = pthread_create(&id, &attr, threadRunFunc, this);
        pthread_attr_destroy(&attr);
        if (r != 0) {
            LOG(FATAL) << "create thread fail";
        }
    }

    template<class _ThreadType = Thread>
    _ThreadType *Thread::current() {
        Thread *thread = localPtr<Thread>().get();
        if (thread) {
            return dynamic_cast<_ThreadType *>(thread);
        }
        auto current = new Thread(pthread_self());
        localPtr<_ThreadType>().reset(current);
        return current;
    }

    void Thread::addExitCallBack(Thread::CallBack callBack) {
        if (Thread::current() == this) {
            exitCallBack.push_back(callBack);
        } else if (status != EXITED) {
            lock_guard g(callBackLock);
            exitCallBack.push_back(callBack);
        } else {
            throw runtime_error("[BUG] thread exited");
        }
    }

    Thread::~Thread() {
        if (current() != this) {
            if (status != EXITED) {
                status = DESTROY;
                // 说明不是new来的，
                LOG(WARNING) << "[bug]不要用局部变量来做耗时的线程。因为pthread会全程拿到对象指针。你可以直接new,不会内存泄露";
                join();
            }
        } else if (status == PROXY) {
            destroyAndInvokeCallback();
        }

    }

    void Thread::destroyAndInvokeCallback() {
        // 说明不是new来的，
        lock_guard guard(callBackLock);
        for (const auto &cb:exitCallBack) {
            cb();
        }
        exitCallBack.clear();
    }

    void Thread::setPriority(unsigned int priority) {
        if (status != INIT) {
            throw runtime_error("[bug] thread started!! setPriority");
        }
        sched_param param = {priority};
        pthread_attr_setschedparam(&attr, &param);
    }

}


