file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.c)
add_library(vdng STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdng
    PUBLIC inc 
    PRIVATE src)

target_link_libraries(vdng
    PUBLIC vdlib flecs)