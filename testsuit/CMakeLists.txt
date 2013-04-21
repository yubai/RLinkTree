cmake_minimum_required(VERSION 2.8)

link_directories(${RTREE_SOURCE_DIR})

add_executable (basic_test basic_test.cc) 
target_link_libraries(basic_test rtree)

add_executable (performance_test performance_test.cc) 

target_link_libraries(performance_test rtree)

add_executable (concurrency_test concurrency_test.cc) 
target_link_libraries(concurrency_test rtree pthread)
