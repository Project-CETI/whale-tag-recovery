set(DIR_NAME "gps")
set(SOURCES 
    gps.c
    minmea.c
    ) 

# set(INCLUDES 
#     gps.h
#     minmea.h
#     ublox_config.h
#     ) 

add_library("gps")
target_sources(${DIR_NAME} PRIVATE ${SOURCES})
# target_include_directories(${DIR_NAME} PUBLIC ${INCLUDES})

target_link_libraries(${DIR_NAME} PRIVATE pico_stdlib pico_time hardware_pio hardware_clocks hardware_rosc hardware_rtc hardware_sleep)

