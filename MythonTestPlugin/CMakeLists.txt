cmake_minimum_required(VERSION 3.19)
project(MythonTestPlugin)

set(CMAKE_CXX_STANDARD 20)

message(STATUS "MythonTestPlugin current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

set(TEST_PLUGIN_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_PLUGIN_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TEST_PLUGIN_HEADERS testplug.h framework.h)
set(TEST_PLUGIN_MODULES
    ${TEST_PLUGIN_MODULES_DIR}/dllmain.cpp
    ${TEST_PLUGIN_MODULES_DIR}/testplug.cpp)

add_library(MythonTestPlugin SHARED ${TEST_PLUGIN_MODULES})
target_include_directories(MythonTestPlugin PRIVATE ${TEST_PLUGIN_HEADERS_DIR} ${KERNEL_HEADERS_SOURCE_DIR})
