FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG d3932b1ba2e8d3936095443b6692580659598175
)

# In Source Libraries
add_subdirectory("utest")
add_subdirectory("luajit")
add_subdirectory("flecs")
add_subdirectory("volk")
FetchContent_MakeAvailable(SDL3)