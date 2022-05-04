cmake_minimum_required(VERSION 3.19)
project(ExternalObjectDemo)

set(CMAKE_CXX_STANDARD 17)

set(MAIN_FILE_EOD ExternalObjectDemo.cpp)
set(LEXER_EOD ../lexer.h ../lexer.cpp)
set(RUNTIME_EOD ../runtime.h ../runtime.cpp)
set(PARSE_EOD ../parse.h ../parse.cpp)
set(STATEMENT_EOD ../statement.h ../statement.cpp)

add_executable(ExternalObjectDemo ${MAIN_FILE_EOD} ${LEXER_EOD} ${RUNTIME_EOD} ${PARSE_EOD} ${STATEMENT_EOD})
