cmake_minimum_required(VERSION 3.19)
project(ExternalObjectDemo)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "ExternalObjectDemo current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ExternalObjectDemo ${CMAKE_CURRENT_SOURCE_DIR}/src/ExternalObjectDemo.cpp)
target_link_libraries(ExternalObjectDemo MythonInterpreterLib)
