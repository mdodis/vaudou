add_executable(vdcli cli.c)
target_link_libraries(vdcli 
    PUBLIC
    libluajit vdlib)

target_include_directories(vdcli
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../ext/luajit/src")

# Copy ${CMAKE_CURRENT_SOURCE_DIR}/builtin to <output directory>/cli/builtin