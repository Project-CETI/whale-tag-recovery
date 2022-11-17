set(DIR_NAME "aprs")
set(SOURCES 
    aprs.c
    vhf.c
    aprs_sleep.c
    ) 

# set(INCLUDES 
#     aprs.h
#     vhf.h
#     ) 

add_library("aprs")
target_sources(${DIR_NAME} PRIVATE ${SOURCES})
# target_include_directories(${DIR_NAME} PUBLIC ${INCLUDES})
pico_generate_pio_header(${DIR_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/uart_tx.pio)

target_link_libraries(${DIR_NAME} PRIVATE pico_stdlib pico_time hardware_pio hardware_clocks hardware_rosc hardware_rtc hardware_sleep)
