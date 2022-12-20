cmake_minimum_required(VERSION 3.19)
project(ExternalObjectDemo)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_EOD ../declares.h ../throw_messages.h ../throw_messages.cpp)
set(MAIN_FILE_EOD ExternalObjectDemo.cpp)
set(LEXER_EOD ../lexer.h ../lexer.cpp)
set(RUNTIME_EOD ../runtime.h ../runtime.cpp)
set(PARSE_EOD ../parse.h ../parse.cpp)
set(STATEMENT_EOD ../statement.h ../statement.cpp)
set(HELPER_EOD
    ../special_objects.h ../special_objects_statement.h ../special_objects.cpp
    ../math_object.h ../math_object_statement.h ../math_object.cpp)

add_executable(ExternalObjectDemo ${COMMON_EOD} ${MAIN_FILE_EOD} ${LEXER_EOD}
               ${RUNTIME_EOD} ${PARSE_EOD} ${STATEMENT_EOD} ${HELPER_EOD})
