cmake_minimum_required(VERSION 3.19)
project(NonStdPrint)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_NSP ../declares.h ../throw_messages.h ../throw_messages.cpp)
set(MAIN_FILE_NSP NonStdPrint.cpp)
set(LEXER_NSP ../lexer.h ../lexer.cpp)
set(RUNTIME_NSP ../runtime.h ../runtime.cpp)
set(PARSE_NSP ../parse.h ../parse.cpp)
set(STATEMENT_NSP ../statement.h ../statement.cpp)
set(HELPER_NSP
    ../special_objects.h ../special_objects_statement.h ../special_objects.cpp
    ../math_object.h ../math_object_statement.h ../math_object.cpp)

add_executable(NonStdPrint ${COMMON_NSP} ${MAIN_FILE_NSP} ${LEXER_NSP} ${RUNTIME_NSP}
               ${PARSE_NSP} ${STATEMENT_NSP} ${HELPER_NSP})
