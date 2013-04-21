cmake_minimum_required(VERSION 2.8)

project(rtree)

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")

SET(SRC_LIST rtree.cc mempool.cc)

add_library(rtree SHARED ${SRC_LIST})

target_link_libraries(rtree pthread)

add_subdirectory(testsuit)