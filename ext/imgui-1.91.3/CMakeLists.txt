file(GLOB IMGUI_SOURCES "imgui/*.cpp")
file(GLOB IMGUI_HEADERS "imgui/*.h")

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

set_target_properties(imgui PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(imgui PUBLIC .)
