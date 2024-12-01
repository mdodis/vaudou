set(VDRHI_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PUBLIC_HEADERS ${VDRHI_PUBLIC_HEADERS}/rhi/*.h)

add_library(vdrhi INTERFACE)

target_link_libraries(vdrhi
    INTERFACE vdlib vdshdc TracyClient)

target_include_directories(vdrhi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdrhi
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/rhi ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdrhi
    DEPENDS ${PUBLIC_HEADERS}
    COMMENT "Copying public headers to output directory"
    VERBATIM
)

add_custom_target(vdrhi_public_headers ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vdrhi)

add_dependencies(vdrhi vdrhi_public_headers)

add_subdirectory("vulkan")
