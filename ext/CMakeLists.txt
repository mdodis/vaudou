FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.0
)

# In Source Libraries
add_subdirectory("utest")
add_subdirectory("luajit")
add_subdirectory("flecs")
add_subdirectory("volk")
add_subdirectory("cglm-0.9.4")
add_subdirectory("amd_vma")
add_subdirectory("glslang")
add_subdirectory("tracy-0.11.1")
add_subdirectory("imgui-1.91.3")
FetchContent_MakeAvailable(SDL3)
