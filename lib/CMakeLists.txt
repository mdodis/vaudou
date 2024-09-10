file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")
add_library(vdlib STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdlib PUBLIC "./")
target_compile_features(vdlib PUBLIC c_std_11)