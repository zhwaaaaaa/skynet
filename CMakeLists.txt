cmake_minimum_required(VERSION 3.8)
project(mesher)

set(CMAKE_CXX_STANDARD 14)

include_directories(src src/event)

set(SOURCE_FILES src/main.cpp
        src/event/event_dispatcher.cpp
        src/event/channel.cpp
        src/event/socket_channel.cpp
        src/event/conn_channel.cpp
        )
add_executable(mesher ${SOURCE_FILES})