cmake_minimum_required(VERSION 3.8)
project(skynet)

set(CMAKE_CXX_STANDARD 14)

include_directories(src src/event)

set(SOURCE_FILES src/main.cpp
        src/event/event_dispatcher.cpp
        src/event/channel.cpp
        src/event/socket_channel.cpp
        src/event/conn_channel.cpp
        src/util/buffer.h
        src/event/endpoint.cpp
        src/event/endpoint.h
        )
add_executable(skynet ${SOURCE_FILES})
target_link_libraries(skynet glog gflags)