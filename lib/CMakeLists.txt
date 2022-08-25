set(BIN_NAME "lib")
set(SOURCES
	minmea.c
	)

add_definitions("-Dtimegm=mktime")

add_library("lib")

target_sources(${BIN_NAME} PRIVATE ${SOURCES})

