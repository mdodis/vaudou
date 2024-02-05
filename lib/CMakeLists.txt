file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")
add_library(vdlib STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdlib PUBLIC "./")