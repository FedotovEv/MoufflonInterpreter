cmake_minimum_required(VERSION 3.19)
project(MythonFilePlugin)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_HEADERS_PLUG_FILE ../declares.h ../throw_messages.h ../runtime.h ../statement.h ../parse.h)
set(MAIN_FILES_PLUG_FILE file_plugin.h framework.h dllmain.cpp file_plugin.cpp)
add_library(MythonFilePlugin SHARED ${COMMON_HEADERS_PLUG_FILE} ${MAIN_FILES_PLUG_FILE})
target_link_library(MythonFilePlugin MythlonInterpreter.lib)
