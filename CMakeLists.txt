cmake_minimum_required(VERSION 3.20)
project(vaudou LANGUAGES C)

include(FetchContent)
include(CTest)

# Add the external libraries
add_subdirectory("ext")
add_subdirectory("lib")
add_subdirectory("ng")
add_subdirectory("ed")
add_subdirectory("test")