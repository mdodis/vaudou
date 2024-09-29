file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")
add_library(vdedlib STATIC ${SOURCES} ${HEADERS})

target_link_library(vdedlib
    PUBLIC vdlib
)