set(VDNG_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB_RECURSE PUBLIC_HEADERS ${VDNG_PUBLIC_HEADERS}/*.h)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)
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

add_custom_command(TARGET vdng POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shd ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shd)

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng
    DEPENDS ${PUBLIC_HEADERS}
    COMMENT "Copying public headers to output directory"
    VERBATIM
)
add_custom_target(vdng_public_headers ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng)

add_dependencies(vdng vdng_public_headers)