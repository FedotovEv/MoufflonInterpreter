cmake_minimum_required(VERSION 3.22.0)

# Имя проекта
project(ExtractEncodingInfo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIncludeFileCXX)
check_include_file_cxx(getopt.h HAVE_GETOPT_HEADER)

set(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/ExtractEncodingInfo.cpp)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

add_executable(${PROJECT_NAME} ${SRC_FILES})

if(NOT HAVE_GETOPT_HEADER)
  message(STATUS "Build ExtractEncodingInfo in NON-POSIX environment")
  set(HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/posix/getopt.h)
  target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/posix)
else()
  message(STATUS "Build ExtractEncodingInfo in POSIX environment")
  target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
