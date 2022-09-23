set(BIN_NAME "recovery")
set(SOURCES
	main.c
	tag.c

	)

set(INCLUDES 
    generated
    ) 


add_executable(${BIN_NAME})

add_custom_command(
	TARGET ${BIN_NAME}
	PRE_BUILD
    COMMAND cd ${CMAKE_SOURCE_DIR}/config-parser/ && python3 -m json2header -i floater.json
)

target_sources(${BIN_NAME} PRIVATE ${SOURCES})
target_include_directories(${BIN_NAME} PUBLIC ${INCLUDES})

add_subdirectory("gps")
add_subdirectory("aprs")

pico_enable_stdio_usb(${BIN_NAME} 1) # Need this to enable picotool via USB connection
#pico_enable_stdio_uart(${BIN_NAME} 0) # Need this so that swarm isn't in bad shape

pico_add_extra_outputs(${BIN_NAME})

target_link_libraries(${BIN_NAME} PRIVATE pico_stdlib pico_time hardware_pio hardware_clocks hardware_rosc hardware_rtc hardware_sleep aprs gps)

