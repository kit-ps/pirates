cmake_minimum_required(VERSION 3.10)

project(PIRATES VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

link_directories(/usr/local/lib/)

message(TEMP="${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/cmake")
find_package(Botan REQUIRED)

add_executable(caller ./src/caller.cpp)
target_include_directories(caller PRIVATE /home/app/LPCNet/src/)
target_link_libraries(caller crypto lpcnetfreedv rpc Botan::Botan)
