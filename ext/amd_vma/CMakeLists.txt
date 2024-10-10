add_library(amd_vma STATIC "vk_mem_alloc.h" "vk_mem_alloc.cpp")
target_include_directories(amd_vma PUBLIC "./")
target_compile_features(amd_vma PRIVATE cxx_std_11)
target_link_libraries(amd_vma PUBLIC volk)