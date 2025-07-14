cmake_minimum_required(VERSION 3.15)

project(xpchatlib VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

# target_include_directories(xpchatlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(xpchatlib src/chat_protocol.cpp)
target_include_directories(xpchatlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_property(TARGET xpchatlib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

