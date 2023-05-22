cmake_minimum_required(VERSION 3.10)

project(PIRATES VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SEAL 4.1 REQUIRED)
find_library(lpcnetfreedv lpcnetfreedv)

#set(SOURCES ./src/worker.cpp ./src/params.h)
#add_executable(worker ${SOURCES})
add_executable(worker ./src/worker.cpp)
#target_include_directories(worker PRIVATE ./src/)
target_link_libraries(worker SEAL::seal_shared crypto lpcnetfreedv rpc)