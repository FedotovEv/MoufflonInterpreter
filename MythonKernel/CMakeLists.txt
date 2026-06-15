
# Проект ядра языка МУФЛОН - его интерпретатор, исполнительская среда, а также набор модульных и интеграционных тестов для него.
cmake_minimum_required(VERSION 3.19)
project(MythonKernel)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-implicit-fallthrough")
endif()

message(STATUS "Kernel current source dir - " ${CMAKE_CURRENT_SOURCE_DIR})

set(LOC_KERNEL_HEADERS_FILES
    debug_context.h
    declares.h
    error_classes.h
    lexer.h
    math_object.h
    math_object_statement.h
    parse.h
    plugin_caller.h
    plugin_helpers.h
    runtime.h
    special_objects.h
    special_objects_statement.h
    statement.h
    throw_messages.h)

set(TEST_HEADERS_FILES test_runner_p.h)

set(LOC_KERNEL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LOC_KERNEL_MODULEs_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(LOC_KERNEL_HEADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LOC_KERNEL_MODULES_FILES
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/error_classes.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/lexer.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/math_object.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/parse.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/plugin_caller.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/runtime.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/special_objects.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/statement.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/throw_messages.cpp)

set(TEST_MODULES_FILES
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/main.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/lexer_test_open.cpp    
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/parse_test.cpp    
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/runtime_test.cpp
    ${LOC_KERNEL_MODULEs_SOURCE_DIR}/statement_test.cpp)

# Некоторые переменные из определённых ниже будут распространены в родительскую область видимости, чтобы их далее могли
# использовать прочие подпроекты данного решения для выяснения маршрутов и наборов файлов, входящих в ядро интерпретатора.
set(KERNEL_SOURCE_DIR ${LOC_KERNEL_SOURCE_DIR} PARENT_SCOPE)
set(KERNEL_MODULEs_SOURCE_DIR ${LOC_KERNEL_MODULEs_SOURCE_DIR} PARENT_SCOPE)
set(KERNEL_HEADERS_SOURCE_DIR ${LOC_KERNEL_HEADERS_SOURCE_DIR} PARENT_SCOPE)
set(KERNEL_HEADERS_FILES ${LOC_KERNEL_HEADERS_FILES} PARENT_SCOPE)
set(KERNEL_MODULES_FILES ${LOC_KERNEL_MODULES_FILES} PARENT_SCOPE)

# Генерируем статическую библиотеку для компоновки с программными комплексами, котороые возжелают нас использовать.
add_library(MythonInterpreterLib STATIC ${LOC_KERNEL_MODULES_FILES})
target_include_directories(MythonInterpreterLib PUBLIC ${LOC_KERNEL_HEADERS_SOURCE_DIR})

# Генерация приложения с модульными и интеграционными тестами МУФЛОНА.
add_executable(MythonInterpreterTest ${TEST_MODULES_FILES})
target_include_directories(MythonInterpreterTest PRIVATE ${KERNEL_HEADERS_SOURCE_DIR})
target_link_libraries(MythonInterpreterTest MythonInterpreterLib)
