file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")

add_library(vdw_sdl ${SOURCES} ${HEADERS})

target_include_directories(vdw_sdl PUBLIC ".")

target_link_libraries(vdw_sdl 
    PUBLIC vdng vdlib flecs
    PRIVATE SDL3::SDL3)

if (APPLE)
    find_package(Vulkan REQUIRED)
    target_link_libraries(vdw_sdl
        PRIVATE Vulkan::Vulkan)
else()
    target_link_libraries(vdw_sdl
        PRIVATE volk)
endif()


