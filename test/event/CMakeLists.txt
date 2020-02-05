add_executable(server_keeper server_keeper_test.cpp )
target_link_libraries(server_keeper glog gflags uv snThread skynetCoreLib)