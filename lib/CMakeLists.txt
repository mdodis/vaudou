file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")
add_library(vdlib STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdlib PUBLIC "./")
target_link_libraries(vdlib PUBLIC cglm)
target_compile_features(vdlib PUBLIC c_std_11)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    if (VD_OPTION_ENABLE_ASAN)
        target_compile_options(vdlib BEFORE PUBLIC "-fsanitize=address")
    endif()
endif()
