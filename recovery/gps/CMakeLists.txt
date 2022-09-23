set(DIR_NAME "gps")
set(SOURCES 
    gps.c
    ) 

add_library("gps")
target_sources(${DIR_NAME} PRIVATE ${SOURCES})

target_link_libraries(${DIR_NAME} PRIVATE pico_stdlib lib)

