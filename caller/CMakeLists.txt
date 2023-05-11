cmake_minimum_required(VERSION 3.10)

project(PIRATES VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#include_directories()
#link_directories()
# Needed to find liblpcnetfreedv.so.0.4
link_directories(/usr/local/lib/)
#link_directories(/home/app/LPCNet/src/)

#find_package(SEAL 3.5 REQUIRED)
find_package(SEAL 4.1 REQUIRED)

#find_library(lpcnetfreedv lpcnetfreedv)
#set(SOURCES ./src/client.cpp /home/app/LPCNet/src/lpcnet_dump.h /home/app/LPCNet/src/lpcnet_quant.h /home/app/LPCNet/src/lpcnet_freedv_internal.h)
set(SOURCES ./src/caller.cpp)
#add_executable(client ./src/client.cpp)
add_executable(caller ${SOURCES})
target_include_directories(caller PRIVATE /home/app/LPCNet/src/)
target_link_libraries(caller SEAL::seal_shared crypto lpcnetfreedv rpc botan)
