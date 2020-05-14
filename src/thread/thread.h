//
// Created by dear on 20-1-30.
//

#ifndef SKYNET_THREAD_H
#define SKYNET_THREAD_H

#include <type_traits>
#include <pthread.h>
#include <utility>
#include <memory>
#include <zconf.h>
#include <vector>
#include <thread>
#include <mutex>
#include <glog/logging.h>
#include <atomic>

namespace sn {

    using namespace std;

    /**
     * 注意：创建线程要用new
     */
    class Thread {
    public:
        enum Status {
            INIT,
            LIVED,
            EXITED,
            DESTROY,
            PROXY
        };

    private:
        template<typename T>
        static unique_ptr<T> &localPtr() {
            thread_local unique_ptr<T> ptr(nullptr);
            return ptr;
        }

    public:
        using ThreadId = pthread_t;
        using ThreadAttr = pthread_attr_t;

        template<class _ThreadType = Thread>
        static _ThreadType *current();

    public:

        template<typename T>
        static void local(T *heapPtr) {
            return localPtr<T>().reset(heapPtr);
        }

        template<typename T>
        static T *localRelease() {
            return localPtr<T>().release();
        }

        template<typename T>
        static T &local() {
            return *localPtr<T>().get();
        }

        static void yield() {
#if defined(OS_MACOSX)
            pthread_yield_np();
#else
            pthread_yield();
#endif
        }


        static bool sleep(unsigned int ms) {
            return usleep(ms * 1000) == 0;
        }

        typedef void (*CallBack)();

    private:
        ThreadId id;
        ThreadAttr attr;
        vector<CallBack> exitCallBack;
        mutable mutex callBackLock;

        volatile Status status;

        explicit Thread(ThreadId id) : id(id), exitCallBack(), attr({0}), callBackLock(), status(PROXY) {
        }

        void destroyAndInvokeCallback();

        static void *threadRunFunc(void *ptr);

    protected:

        explicit Thread();

        virtual void run() {}

    public:

        Thread(Thread &) = delete;

        Thread(Thread &&) = delete;

        Thread &operator=(const Thread &) = delete;

        template<class Thread = Thread>
        bool operator==(const Thread &other) {
            return getId() == other.getId();
        }

        virtual ~Thread();
#ifndef OS_MACOSX
    private:
        // _GLIBCXX_RESOLVE_LIB_DEFECTS
        // 2097.  packaged_task constructors should be constrained
        template<typename _Tp>
        using __not_same = __not_<is_same<
                typename remove_cv<typename remove_reference<_Tp>::type>::type,
                Thread>>;

        template<typename T>
        struct ParamWrapper {
            T invoker;
            Thread *thread;
        };

        template<typename _InvokerType>
        static void *threadRunInParamConstructor(void *p) {
            using _WT = ParamWrapper<_InvokerType>;
            _WT *wPtr = static_cast<_WT *>(p);
            wPtr->thread->status = LIVED;
            localPtr<Thread>().reset(wPtr->thread);
            wPtr->invoker();
            wPtr->thread->status = DESTROY;
            wPtr->thread->destroyAndInvokeCallback();
            delete wPtr;
            return nullptr;
        }

    public:

        template<typename _Callable, typename... _Args,
                typename = _Require<__not_same<_Callable>>>

        explicit Thread(_Callable &&__f, _Args &&... __args):id(0), status(INIT) {
            static_assert(__is_invocable<typename decay<_Callable>::type,
                                  typename decay<_Args>::type...>::value,
                          "std::thread arguments must be invocable after conversion to rvalues"
            );
            //
            pthread_attr_init(&attr);
            // 如果是临时调用，把它做成分离状态.
            join();

            auto invoker = thread::__make_invoker(__f, __args...);
            using _InvokerType = decltype(thread::__make_invoker(__f, __args...));
            const auto wrapper = new ParamWrapper<_InvokerType>({thread::__make_invoker(__f, __args...), this});
            int r = pthread_create(&id, nullptr, threadRunInParamConstructor<_InvokerType>, wrapper);
            if (r != 0) {
                LOG(FATAL) << "create thread fail";
            }
        }
#endif
    public:
        virtual void join() const final;

        virtual ThreadId getId() const final;

        virtual void detach() final;

        virtual void start() final;

        void setPriority(unsigned int priority);

        void addExitCallBack(CallBack callBack);
    };


}


#endif //SKYNET_THREAD_H
