
file(GLOB_RECURSE SOURCES "*.c")
add_executable(vdtestrunner ${SOURCES})
target_link_libraries(vdtestrunner vdlib utest)

add_test(NAME vdtest COMMAND vdtestrunner)
set_property(
    TEST vdtest
    PROPERTY PASS_REGULAR_EXPRESSION "[  PASSED  ]")