
file(GLOB_RECURSE SOURCES "*.c")
add_executable(vdtestrunner ${SOURCES})
target_link_libraries(vdtestrunner PRIVATE vdlib utest)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    if (VD_OPTION_ENABLE_ASAN)
        target_compile_options(vdtestrunner BEFORE PUBLIC "-fsanitize=address")
    endif()
endif()

add_test(NAME vdtest COMMAND vdtestrunner)
set_property(
    TEST vdtest
    PROPERTY FAIL_REGULAR_EXPRESSION "^\[  FAILED  \]$")
