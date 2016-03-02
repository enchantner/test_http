cmake_minimum_required(VERSION 2.8)
project(FINAL)

set(CMAKE_CXX_FLAGS "-Wall")

add_executable(final nthinks.c)

TARGET_LINK_LIBRARIES(final pthread)
