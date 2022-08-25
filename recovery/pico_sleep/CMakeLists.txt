set(DIR_NAME "pico_sleep")
set(SOURCES 
    sleep.c
    ) 

# set(INCLUDES 
#     sleep.h
#     ) 

add_library("pico_sleep")
target_sources(${DIR_NAME} PRIVATE ${SOURCES})
# target_include_directories(${DIR_NAME} PUBLIC ${INCLUDES})

target_link_libraries(${DIR_NAME} PRIVATE pico_stdlib pico_time hardware_pio hardware_clocks hardware_rosc hardware_rtc hardware_sleep)
