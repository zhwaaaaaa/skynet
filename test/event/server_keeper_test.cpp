//
// Created by dear on 2020/2/5.
//
#include <iostream>
#include <service_keeper.h>
#include <thread/thread.h>
#include <registry/naming_server.h>
#include <event/client.h>

using namespace sn;

void testSk() {
    Client client;
    Thread::local<NamingServer>(new DemoNamingServer);
    ServiceKeeper sk(string("com.zhw.PrintService.print"));


}

int main() {
    testSk();
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
