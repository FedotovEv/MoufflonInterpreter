cmake_minimum_required(VERSION 3.19)
# Этот проект - программа испытания в работе втыкалы MythonFilePlugin, написанной на чистом C.
project(TestFilePlugin)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "TestFilePlugin current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(TestFilePlugin ${CMAKE_CURRENT_SOURCE_DIR}/src/TestFilePlugin.cpp)
target_link_libraries(TestFilePlugin MythonInterpreterLib)
