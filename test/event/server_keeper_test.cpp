//
// Created by dear on 2020/2/5.
//
#include <iostream>
#include <service_keeper.h>
#include <thread/thread.h>
#include <registry/naming_server.h>
#include <event/client.h>

using namespace sn;


int main() {
    Client client;
    Thread::local<NamingServer>(new DemoNamingServer);
    ServiceKeeper sk(string("com.zhw.PrintService.print"));
    // 执行一次 close 的回调函数才会执行
    uv_run(uv_default_loop(), UV_RUN_ONCE);
}
