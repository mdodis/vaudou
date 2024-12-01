
add_library(vdrhi-vulkan STATIC "rhi_vulkan.h" "rhi_vulkan.c")

target_include_directories(vdrhi-vulkan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(vdrhi-vulkan
    PUBLIC vdlib vdrhi
    PRIVATE vdshdc volk amd_vma TracyClient)

target_compile_definitions(vdrhi-vulkan PRIVATE VK_NO_PROTOTYPES)
