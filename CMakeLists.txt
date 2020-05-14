cmake_minimum_required(VERSION 3.8)
project(skynet)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(DEPS skynet glog gflags uv pthread)

if (CMAKE_HOST_APPLE)
    add_definitions(-DOS_MACOSX)

    include_directories(/usr/local/Cellar/glog/0.4.0/include)
    link_directories(/usr/local/Cellar/glog/0.4.0/lib)

    include_directories(/usr/local/Cellar/libuv/1.37.0/include)
    link_directories(/usr/local/Cellar/libuv/1.37.0/lib)

    include_directories(/usr/local/Cellar/gflags/2.2.2/include)
    link_directories(/usr/local/Cellar/gflags/2.2.2/lib)

    include_directories(/usr/local/Cellar/boost/1.72.0_2/include)
    link_directories(/usr/local/Cellar/boost/1.72.0_2/lib)
else ()
    # add_definitions(-DUSE_ZK)
#    list(APPEND DEPS tcmalloc_minimal)

    if (DEFINED USE_ZK)
        message(===================================)
        list(APPEND SOURCE_FILES src/registry/ZkNamingServer.cpp)
        add_definitions(-DUSE_ZK)
        list(APPEND DEPS zookeeper_mt)
    endif ()

endif ()

include_directories(src)

set(SOURCE_SUB_DIR event nevent thread util)
foreach (dir ${SOURCE_SUB_DIR})
    file(GLOB cur_files src/${dir}/*.cpp)
    LIST(APPEND SOURCE_FILES ${cur_files})
endforeach ()


list(APPEND SOURCE_FILES src/registry/NamingServerFactory.cpp)

MESSAGE(${SOURCE_FILES})

add_executable(skynet src/main.cpp ${SOURCE_FILES})
target_link_libraries(skynet glog gflags uv pthread)

add_subdirectory("test/thread")
add_subdirectory("test/event")
add_subdirectory("test/util")
add_subdirectory("test/zookeeper")
