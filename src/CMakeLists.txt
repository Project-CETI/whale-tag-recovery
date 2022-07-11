cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(ceti_recovery_board_sdk C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME recovery)
set(SOURCES ${PROJECT_SOURCE_DIR}/main.c ${PROJECT_SOURCE_DIR}/aprs.c ${PROJECT_SOURCE_DIR}/vhf.c ${PROJECT_SOURCE_DIR}/swarm.c ${PROJECT_SOURCE_DIR}/tag.c)
pico_sdk_init()

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

pico_enable_stdio_usb(${PROJECT_NAME} 1) # Need this to enable picotool via USB connection
pico_enable_stdio_uart(${PROJECT_NAME} 0) # Need this so that swarm isn't in bad shape

pico_generate_pio_header(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/uart_tx.pio)
pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib pico_time hardware_pio)
