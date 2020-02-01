cmake_minimum_required(VERSION 3.8)
project(skynet)

set(CMAKE_CXX_STANDARD 17)

include_directories(src src/event)

set(SOURCE_FILES src/main.cpp
        src/event/event_dispatcher.cpp
        src/event/channel.cpp
        src/event/socket_channel.cpp
        src/event/conn_channel.cpp
        src/event/tcp_channel.cpp
        src/event/tcp_channel.h
        src/event/shake_hands_channel.cpp
        src/event/shake_hands_channel.h
        src/event/server.h
        src/event/server.cpp
        src/event/reactor.h
        src/event/reactor.cpp
        #registry
        src/registry/service_registry.cpp
        src/registry/service_registry.h
        src/registry/naming_server.h
        src/registry/naming_server.cpp
        #util
        src/util/buffer.h
        src/util/endpoint.cpp
        src/util/endpoint.h
        src/util/HASH_MAP_HPP.h
        src/util/func_time.h
        src/util/func_time.cpp
        )

set(THREAD_FILES
        #thread
        src/thread/thread.cpp
        src/thread/thread.h)

add_library(snThread ${THREAD_FILES})
target_link_libraries(snThread glog gflags pthread)

add_executable(skynet ${SOURCE_FILES})
target_link_libraries(skynet glog gflags snThread)

add_subdirectory("test/thread")