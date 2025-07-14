cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

project(xpchatter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

add_subdirectory(../xpchat-lib ${CMAKE_BINARY_DIR}/bin/xpchat-lib)

add_executable(xpchatter src/main.cpp src/chat.cpp src/central.cpp)
target_link_libraries(xpchatter PRIVATE xpchatlib)
target_link_options(xpchatter PRIVATE -lwsock32 -lws2_32)

set_property(TARGET xpchatter PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

