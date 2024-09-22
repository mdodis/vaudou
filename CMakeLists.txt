cmake_minimum_required(VERSION 3.20)
project(vaudou LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

include(FetchContent)
include(CTest)

option(VD_OPTION_ENABLE_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)

set(VD_TRIPLET $<$<CONFIG:Debug>:debug>$<$<CONFIG:Release>:release>-$<$<PLATFORM_ID:Darwin>:osx>$<$<PLATFORM_ID:Linux>:linux>$<$<PLATFORM_ID:Windows>:win>$<$<CONFIG:Debug>:64>$<$<CONFIG:Release>:64>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out-${VD_TRIPLET}/lib)

if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
endif()


# Add the external libraries
add_subdirectory("ext")
add_subdirectory("lib")
add_subdirectory("cli")
add_subdirectory("ng")
add_subdirectory("ed")
add_subdirectory("test")