file(GLOB SOURCES "vd-imgui/*.cpp" "vd-imgui/*.c")
file(GLOB HEADERS "vd-imgui/*.h")

add_library(vd-imgui STATIC ${SOURCES} ${HEADERS})
target_include_directories(vd-imgui PUBLIC .)
target_link_libraries(vd-imgui PUBLIC vdng vdlib flecs imgui)

file(GLOB PRECOMP_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/vd-imgui/shd/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/vd-imgui/shd/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/vd-imgui/shd/*.glsl
)

foreach(input_file ${PRECOMP_SHADERS})
    get_filename_component(file_basename ${input_file} NAME_WE)
    get_filename_component(file_ext ${input_file} EXT)
    
    set(output_file ${CMAKE_CURRENT_SOURCE_DIR}/vd-imgui/shd/generated/${file_basename}${file_ext})

    add_custom_command(
        OUTPUT  ${output_file}
        COMMAND ${CMAKE_COMMAND} -E echo "Converting ${input_file}"
        COMMAND luajit::cli ${VD_SCRIPTS_DIRECTORY}/cv_text_to_literal.lua ${input_file} ${output_file}
        DEPENDS ${input_file}
        COMMENT "Generating C string from ${input_file}"
    )

    target_sources(vd-imgui PRIVATE ${output_file})
endforeach()
