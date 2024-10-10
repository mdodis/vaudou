file(GLOB SOURCES "vd-imgui/*.cpp" "vd-imgui/*.c")
file(GLOB HEADERS "vd-imgui/*.h")

add_library(vd-imgui STATIC ${SOURCES} ${HEADERS})
target_include_directories(vd-imgui PUBLIC .)
target_link_libraries(vd-imgui PUBLIC vdlib flecs imgui)