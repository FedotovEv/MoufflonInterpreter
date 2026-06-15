cmake_minimum_required(VERSION 3.19)
project(MythonFilePlugin)

set(CMAKE_C_STANDARD 17)

message(STATUS "MythonFilePlugin current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

set(FILE_PLUGIN_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(FILE_PLUGIN_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(FILE_PLUGIN_MODULES
    ${FILE_PLUGIN_MODULES_DIR}/dllmain.c
    ${FILE_PLUGIN_MODULES_DIR}/file_plugin.c)

add_library(MythonFilePlugin SHARED ${FILE_PLUGIN_MODULES})
target_include_directories(MythonFilePlugin PRIVATE ${FILE_PLUGIN_HEADERS_DIR} ${KERNEL_HEADERS_SOURCE_DIR})
