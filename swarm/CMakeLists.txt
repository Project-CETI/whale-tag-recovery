set(BIN_NAME "swarm")
set(SOURCES
	main.c
	aprs.c
	vhf.c
	swarm.c
	tag.c
	)

add_executable(${BIN_NAME})

add_definitions("-Dtimegm=mktime")

# add_library(minmea SHARED IMPORTED) # or STATIC instead of SHARED
# set_target_properties(minmea PROPERTIES
#   IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../minmea/minmea.so"
#   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../include/minmea"
# )

target_sources(${BIN_NAME} PRIVATE ${SOURCES})

pico_enable_stdio_usb(${BIN_NAME} 1) # Need this to enable picotool via USB connection
pico_enable_stdio_uart(${BIN_NAME} 0) # Need this so that swarm isn't in bad shape

pico_generate_pio_header(${BIN_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/uart_tx.pio)
pico_add_extra_outputs(${BIN_NAME})

target_link_libraries(${BIN_NAME} PRIVATE pico_stdlib pico_time hardware_pio)
