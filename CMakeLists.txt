cmake_minimum_required(VERSION 2.8)
project(FINAL)

set(CMAKE_CXX_FLAGS "-Wall")

file(MAKE_DIRECTORY final)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/final)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/final)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/final)

add_executable(final nthinks.c)

TARGET_LINK_LIBRARIES(final pthread)
