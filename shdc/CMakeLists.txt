file(GLOB_RECURSE SOURCES *.c)
add_library(vdshdc STATIC ${SOURCES})

target_link_libraries(vdshdc
    PUBLIC vdlib 
    PRIVATE volk amd_vma glslang)


target_include_directories(vdshdc PRIVATE ${PROJECT_SOURCE_DIR}/ext/glslang-15.0.0/glslang/Include)