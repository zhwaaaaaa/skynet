cmake_minimum_required(VERSION 3.8)
project(skynet)

set(CMAKE_CXX_STANDARD 17)

include_directories(src src/event)

set(SOURCE_FILES src/main.cpp
        src/event/reactor.h
        src/event/reactor.cpp
        #registry
        src/registry/naming_server.h
        src/registry/naming_server.cpp
        #util
        src/util/buffer.h
        src/util/endpoint.cpp
        src/util/endpoint.h
        src/util/HASH_MAP_HPP.h
        src/util/func_time.h
        src/util/func_time.cpp
        # new event
        src/nevent/channel.cpp
        src/nevent/channel.h
        src/nevent/channel_handler.cpp
        src/nevent/channel_handler.h
        src/nevent/io_error.h
        src/nevent/stream_channel.cpp
        src/nevent/stream_channel.h
        src/nevent/tcp_listener.cpp
        src/nevent/tcp_listener.h)

set(THREAD_FILES
        #thread
        src/thread/thread.cpp
        src/thread/thread.h)

add_library(snThread ${THREAD_FILES})
target_link_libraries(snThread glog gflags pthread)

add_executable(skynet ${SOURCE_FILES})
target_link_libraries(skynet glog gflags snThread uv)

add_subdirectory("test/thread")