add_executable(vdcli cli.c)
target_link_libraries(vdcli 
    PUBLIC
    luajit vdlib)

LINK_LUAJIT(vdcli)
# Copy ${CMAKE_CURRENT_SOURCE_DIR}/builtin to <output directory>/cli/builtin
add_custom_command(TARGET vdcli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/builtin
    $<TARGET_FILE_DIR:vdcli>/builtin)