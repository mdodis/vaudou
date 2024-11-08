set(VDNG_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/inc)

file(GLOB_RECURSE PUBLIC_HEADERS ${VDNG_PUBLIC_HEADERS}/*.h)

file(GLOB_RECURSE SOURCES *.c)
file(GLOB_RECURSE HEADERS *.h)
add_library(vdng STATIC ${SOURCES} ${HEADERS})

target_include_directories(vdng
    PUBLIC inc 
    PRIVATE src .)

target_link_libraries(vdng
    PUBLIC vdlib flecs amd_vma 
    PRIVATE vdshdc volk glslang TracyClient)

target_compile_definitions(vdng PRIVATE VK_NO_PROTOTYPES)

if (VD_OPTION_ENABLE_VALIDATION_LAYERS)
    target_compile_definitions(vdng PRIVATE VD_ENABLE_VALIDATION_LAYERS)
endif()

if (VD_OPTION_ENABLE_VULKAN_OBJECT_NAMES)
    target_compile_definitions(vdng PRIVATE VD_ENABLE_VULKAN_OBJECT_NAMES)
endif()

if (VD_OPTION_ENABLE_VMA_TRACKING)
    target_compile_definitions(vdng PRIVATE VD_OPTION_ENABLE_VMA_TRACKING)
endif()

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng
    DEPENDS ${PUBLIC_HEADERS}
    COMMENT "Copying public headers to output directory"
    VERBATIM
)
add_custom_target(vdng_public_headers ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdng)

add_dependencies(vdng vdng_public_headers)


file(GLOB PRECOMP_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/shd/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shd/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shd/*.glsl
)

# get_target_property(LUA_EXECUTABLE luajit::cli LOCATION)
foreach(input_file ${PRECOMP_SHADERS})
    get_filename_component(file_basename ${input_file} NAME_WE)
    get_filename_component(file_ext ${input_file} EXT)
    

    set(output_file ${CMAKE_CURRENT_SOURCE_DIR}/shd/generated/${file_basename}${file_ext})

    add_custom_command(
        OUTPUT  ${output_file}
        COMMAND ${CMAKE_COMMAND} -E echo "Converting ${input_file}"
        COMMAND luajit::cli ${VD_SCRIPTS_DIRECTORY}/cv_text_to_literal.lua ${input_file} ${output_file}
        DEPENDS ${input_file}
        COMMENT "Generating C string from ${input_file}"
    )

    target_sources(vdng PRIVATE ${output_file})
endforeach()
