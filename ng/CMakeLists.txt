file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.c)
add_library(vdng STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdng
    PUBLIC inc 
    PRIVATE src)

target_link_libraries(vdng
    PUBLIC vdlib flecs
    PRIVATE volk amd_vma glslang)

if (VD_OPTION_ENABLE_VALIDATION_LAYERS)
    target_compile_definitions(vdng PRIVATE VD_ENABLE_VALIDATION_LAYERS)
endif()

if (VD_OPTION_ENABLE_VULKAN_OBJECT_NAMES)
    target_compile_definitions(vdng PRIVATE VD_ENABLE_VULKAN_OBJECT_NAMES)
endif()