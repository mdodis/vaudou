file(GLOB_RECURSE SOURCES "*.c")
file(GLOB_RECURSE HEADERS "*.h")

add_executable(vded ${SOURCES} ${HEADERS})

target_include_directories(vded PRIVATE ".")

target_link_libraries(vded 
    PUBLIC vdng vdlib SDL3::SDL3
    PRIVATE volk vdw_sdl vd-imgui)
