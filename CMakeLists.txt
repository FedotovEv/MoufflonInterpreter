cmake_minimum_required(VERSION 3.19)
project(MythonInterpreter)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /JMC")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -Wno-unused-parameter -Wno-implicit-fallthrough")
endif()

add_subdirectory(MythonKernel)
add_subdirectory(ExternalObjectDemo)
add_subdirectory(MythonInclude)
add_subdirectory(NonStdPrint)
add_subdirectory(MythonTestPlugin)
add_subdirectory(MythonFilePlugin)
add_subdirectory(TestFilePlugin)
add_subdirectory(ExtractEncodingInfo)

# Отладчик на данный момент успешно не собирается, поэтому пока директива его сборки отключена
#add_subdirectory(MythonDebugger)
