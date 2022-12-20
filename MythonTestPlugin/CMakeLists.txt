cmake_minimum_required(VERSION 3.19)
project(MythonTestPlugin)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_HEADERS_PLUG_TST ../declares.h ../throw_messages.h ../runtime.h ../statement.h ../parse.h)
set(MAIN_FILES_PLUG_TST testplug.h framework.h dllmain.cpp testplug.cpp)
add_library(MythonTestPlugin SHARED ${COMMON_HEADERS_PLUG_TST} ${MAIN_FILES_PLUG_TST})
target_link_library(MythonTestPlugin MythlonInterpreter.lib)
