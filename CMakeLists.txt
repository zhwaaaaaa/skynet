cmake_minimum_required(VERSION 3.8)
project(skynet)

set(CMAKE_CXX_STANDARD 17)

include_directories(src src/event)

set(SOURCE_FILES
        src/event/reactor.h
        src/event/reactor.cpp
        src/event/client.cpp
        src/event/client.h
        src/event/service_keeper.cpp
        src/event/service_keeper.h
        src/event/channel_keeper.cpp
        src/event/channel_keeper.h
        src/event/server.cpp
        src/event/server.h
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
        src/util/byte_buf.h
        src/util/byte_buf.cpp
        #third_part
        src/util/third_part/superfasthash/superfasthash.c
        # new event
        src/nevent/channel.cpp
        src/nevent/channel.h
        src/nevent/channel_handler.cpp
        src/nevent/channel_handler.h
        src/nevent/io_error.h
        src/nevent/stream_channel.cpp
        src/nevent/stream_channel.h
        src/nevent/tcp_listener.cpp
        src/nevent/tcp_listener.h
        src/nevent/transfer_handler.cpp
        src/nevent/transfer_handler.h
        src/nevent/shake_hands_handler.cpp
        src/nevent/shake_hands_handler.h)

set(THREAD_FILES
        #thread
        src/thread/thread.cpp
        src/thread/thread.h)

add_library(snThread ${THREAD_FILES})
target_link_libraries(snThread glog gflags pthread)

add_library(skynetCoreLib ${SOURCE_FILES})
target_link_libraries(skynetCoreLib glog gflags snThread uv)

add_executable(skynet src/main.cpp)
target_link_libraries(skynet skynetCoreLib)

add_subdirectory("test/thread")
add_subdirectory("test/event")
