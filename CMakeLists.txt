cmake_minimum_required(VERSION 3.19)
project(MythonInterpreter)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-implicit-fallthrough")
endif()

add_subdirectory(MythonTestPlugin)
add_subdirectory(MythonFilePlugin)
add_subdirectory(ExternalObjectDemo)
add_subdirectory(NonStdPrint)
add_subdirectory(MythonInclude)

set(COMMON declares.h throw_messages.h throw_messages.cpp)
set(MAIN_FILE main.cpp)
set(LEXER lexer.h lexer.cpp)
set(RUNTIME runtime.h runtime.cpp)
set(PARSE parse.h parse.cpp)
set(STATEMENT statement.h statement.cpp)
set(HELPER
    special_objects.h special_objects_statement.h special_objects.cpp
    math_object.h math_object_statement.h math_object.cpp)
set(TESTS test_runner_p.h lexer_test_open.cpp runtime_test.cpp parse_test.cpp statement_test.cpp)

add_executable(MythonInterpreter ${COMMON} ${MAIN_FILE} ${LEXER} ${RUNTIME} ${PARSE} ${STATEMENT} ${HELPER} ${TESTS})
