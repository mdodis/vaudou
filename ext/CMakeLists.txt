# Fetch Content Libaries
set(SDL_GIT_VERSION release-2.26.5)

FetchContent_Declare(
  sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG ${SDL_GIT_VERSION}
)

# In Source Libraries
add_subdirectory("utest")
add_subdirectory("luajit")