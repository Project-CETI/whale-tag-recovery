cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_FILE test)
pico_sdk_init()

add_executable(${PROJECT_FILE} ${PROJECT_FILE}.c)

pico_enable_stdio_usb(${PROJECT_FILE} 1)
pico_enable_stdio_uart(${PROJECT_FILE} 0)

pico_enable_stdio_usb(${PROJECT_FILE} 1)
pico_enable_stdio_uart(${PROJECT_FILE} 0)

pico_add_extra_outputs(${PROJECT_FILE})

target_link_libraries(${PROJECT_FILE} pico_stdlib)
