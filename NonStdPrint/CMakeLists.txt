cmake_minimum_required(VERSION 3.19)
project(NonStdPrint)

set(CMAKE_CXX_STANDARD 17)

set(MAIN_FILE_NSP NonStdPrint.cpp)
set(LEXER_NSP ../lexer.h ../lexer.cpp)
set(RUNTIME_NSP ../runtime.h ../runtime.cpp)
set(PARSE_NSP ../parse.h ../parse.cpp)
set(STATEMENT_NSP ../statement.h ../statement.cpp)

add_executable(NonStdPrint ${MAIN_FILE_NSP} ${LEXER_NSP} ${RUNTIME_NSP} ${PARSE_NSP} ${STATEMENT_NSP})
