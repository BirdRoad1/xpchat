cmake_minimum_required(VERSION 3.15)

project(xpchatterserver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

add_subdirectory(../xpchat-lib ${CMAKE_BINARY_DIR}/bin/xpchat-lib)

# target_link_libraries(xpchatterserver PRIVATE xpchatlib)
add_executable(xpchatterserver src/main.cpp src/central.cpp src/server.cpp)
target_link_libraries(xpchatterserver PRIVATE xpchatlib)
# set_property(TARGET xpchatterserver PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

