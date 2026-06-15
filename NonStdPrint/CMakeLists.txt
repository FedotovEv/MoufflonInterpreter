cmake_minimum_required(VERSION 3.19)
project(NonStdPrint)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "NonStdPrint current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(NonStdPrint ${CMAKE_CURRENT_SOURCE_DIR}/src/NonStdPrint.cpp)
target_link_libraries(NonStdPrint MythonInterpreterLib)
