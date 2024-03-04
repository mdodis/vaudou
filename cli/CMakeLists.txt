add_executable(vdcli cli.c)
target_link_libraries(vdcli 
    PUBLIC
    libluajit vdlib)

target_include_directories(vdcli
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../ext/luajit/src")

# Copy ${CMAKE_CURRENT_SOURCE_DIR}/builtin to <output directory>/cli/builtin
add_custom_command(TARGET vdcli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/builtin
    $<TARGET_FILE_DIR:vdcli>/builtin)