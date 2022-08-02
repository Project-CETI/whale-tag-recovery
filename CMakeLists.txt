cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project("RecoverCETI" C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

pico_sdk_init()
add_subdirectory("recovery")
add_subdirectory("swarm")
