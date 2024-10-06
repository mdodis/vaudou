cmake_minimum_required(VERSION 3.20)
project(vaudou-tools LANGUAGES C CXX)

set(VD_TRIPLET $<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>-$<$<PLATFORM_ID:Darwin>:osx>$<$<PLATFORM_ID:Linux>:linux>$<$<PLATFORM_ID:Windows>:win>$<$<CONFIG:Debug>:64>$<$<CONFIG:Release>:64>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)

add_subdirectory("../ext/tracy-0.11.1/profiler" tracy)