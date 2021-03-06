//
// Created by dear on 20-1-30.
//
#include <thread/thread.h>
#include <iostream>

void testCurrentThread();

void testLocal(int initSize);

using namespace sn;

class MyThread : public Thread {
private:
    Thread::ThreadId outId;
public:
    explicit MyThread(ThreadId id) : outId(id) {}

public:
    void run() override {
        LOG(INFO) << outId << " ==? " << getId();
    }
};

void testExtendsThread() {
    const auto thread = new MyThread(Thread::current()->getId());
    thread->start();
    thread->join();
};

void testCommon() {

    const auto thread = new Thread([](int s, long l) {
        LOG(INFO) << "=====testCommon 在线程中执行" << s;
        const auto pThread = Thread::current();
        pThread->addExitCallBack([]() {
            LOG(INFO) << "===testCommon" << Thread::current()->getId() << "退出了";
        });
    }, 100, 10000L);
    thread->join();
}

int main() {
    testCurrentThread();
    cout << endl << "======================================================" << endl << endl;
    testExtendsThread();
    cout << endl << "======================================================" << endl << endl;
    testCommon();
    cout << endl << "======================================================" << endl << endl;

    testLocal(100);


}


void testLocal(int initSize) {

    Thread::local<int>(new int(initSize));
    pthread_t pid;
    pthread_create(&pid, nullptr, [](void *p) {
        Thread::local<int>(new int(1000));
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        Thread::current()->addExitCallBack([](){
            LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
            LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
        });
        return p;
    }, nullptr);
    pthread_detach(pid);

    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
    LOG(INFO) << "=====Thread::local<int>()" << ++Thread::local<int>();
}

void testCurrentThread() {
    Thread *current = Thread::current();
    current->addExitCallBack([]() {
        LOG(INFO) << "=====线程退出了";
    });
    current->addExitCallBack([]() {
        LOG(INFO) << "=====线程退出了";
    });

    LOG(INFO) << "线程退出了吗";
}
