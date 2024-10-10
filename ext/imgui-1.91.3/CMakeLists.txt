file(GLOB IMGUI_SOURCES "imgui/*.cpp")
file(GLOB IMGUI_HEADERS "imgui/*.h")

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_include_directories(imgui PUBLIC .)