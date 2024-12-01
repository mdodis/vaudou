cmake_minimum_required(VERSION 3.20)
project(vaudou LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)

include(FetchContent)
include(CTest)

option(VD_OPTION_ENABLE_VALIDATION_LAYERS "Enable Vulkan validation layers" ON)
option(VD_OPTION_ENABLE_VULKAN_OBJECT_NAMES "Enable Vulkan object names" ON)
option(VD_OPTION_ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(VD_OPTION_ENABLE_VMA_TRACKING, "Enable memory tracking" OFF)

message(STATUS "VD_OPTION_ENABLE_VALIDATION_LAYERS: ${VD_OPTION_ENABLE_VALIDATION_LAYERS}")
message(STATUS "VD_OPTION_ENABLE_VULKAN_OBJECT_NAMES: ${VD_OPTION_ENABLE_VULKAN_OBJECT_NAMES}")
message(STATUS "VD_OPTION_ENABLE_ASAN: ${VD_OPTION_ENABLE_ASAN}")
message(STATUS "VD_OPTION_ENABLE_VMA_TRACKING: ${VD_OPTION_ENABLE_VMA_TRACKING}")

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

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Add the external libraries
add_subdirectory("ext")

set(VD_SCRIPTS_DIRECTORY ${PROJECT_SOURCE_DIR}/scripts)

add_subdirectory("lib")
add_subdirectory("shdc")
add_subdirectory("rhi")
add_subdirectory("ng")

add_subdirectory("modules")
add_subdirectory("w_sdl")

add_subdirectory("cli")
add_subdirectory("ed")
add_subdirectory("test")
