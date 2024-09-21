file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.c)
add_library(vdng STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdng
    PUBLIC inc 
    PRIVATE src)

target_link_libraries(vdng
    PUBLIC vdlib flecs
    PRIVATE volk)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(vdng
        PRIVATE VD_ENABLE_VALIDATION_LAYERS)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
endif()