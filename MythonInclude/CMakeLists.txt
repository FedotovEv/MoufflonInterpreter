cmake_minimum_required(VERSION 3.19)
project(MythonInclude)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_MYIN ../declares.h ../throw_messages.h ../throw_messages.cpp)
set(MAIN_FILE_MYIN MythonInclude.cpp)
set(LEXER_MYIN ../lexer.h ../lexer.cpp)
set(RUNTIME_MYIN ../runtime.h ../runtime.cpp)
set(PARSE_MYIN ../parse.h ../parse.cpp)
set(STATEMENT_MYIN ../statement.h ../statement.cpp)
set(HELPER_MYIN
    ../special_objects.h ../special_objects_statement.h ../special_objects.cpp
    ../math_object.h ../math_object_statement.h ../math_object.cpp)

add_executable(MythonInclude ${COMMON_MYIN} ${MAIN_FILE_MYIN} ${LEXER_MYIN} ${RUNTIME_MYIN}
               ${PARSE_MYIN} ${STATEMENT_MYIN} ${HELPER_MYIN})
