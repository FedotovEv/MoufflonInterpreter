cmake_minimum_required(VERSION 3.19)
project(MythonInclude)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "MythonInclude current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(MythonInclude ${CMAKE_CURRENT_SOURCE_DIR}/src/MythonInclude.cpp)
target_link_libraries(MythonInclude MythonInterpreterLib)
