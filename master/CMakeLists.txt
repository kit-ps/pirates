cmake_minimum_required(VERSION 3.10)

project(PIRATES VERSION 0.1 LANGUAGES CXX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SEAL 3.5 REQUIRED)
find_library(lpcnetfreedv lpcnetfreedv)

add_executable(master ./src/master.cpp)
target_link_libraries(master SEAL::seal crypto lpcnetfreedv)