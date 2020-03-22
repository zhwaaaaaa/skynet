add_executable(server_keeper server_keeper_test.cpp)
target_link_libraries(server_keeper glog gflags uv snThread skynetCoreLib)

add_executable(buf buf.cpp ../../src/nevent/IoBuf.cpp ../../src/nevent/IoBuf.h)
target_link_libraries(buf tcmalloc_minimal glog)