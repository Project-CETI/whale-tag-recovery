# set(BASE_DIR "${PROJECT_SOURCE_DIR}")

set(DIR_NAME "gps")
set(SOURCES 
    gps.c
    ) 

add_library("gps")
# add_subdirectory("${BASE_DIR}/lib/")
target_sources(${DIR_NAME} PRIVATE ${SOURCES})

target_link_libraries(${DIR_NAME} PRIVATE pico_stdlib lib)

