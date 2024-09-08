cmake_minimum_required(VERSION 3.20)
project(vaudou LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(CTest)

set(VD_TRIPLET $<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>-$<$<PLATFORM_ID:Darwin>:osx>$<$<PLATFORM_ID:Linux>:linux>$<$<PLATFORM_ID:Windows>:win>-$<$<CONFIG:Debug>:64>$<$<CONFIG:Release>:64>)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)

# Add the external libraries
add_subdirectory("ext")
add_subdirectory("lib")
add_subdirectory("cli")
add_subdirectory("ng")
add_subdirectory("ed")
add_subdirectory("test")