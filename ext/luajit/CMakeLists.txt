if (WIN32)

    add_library(luajit STATIC IMPORTED GLOBAL)
    set_target_properties(luajit PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/win64/luajit.lib"
    )
    target_link_libraries(luajit INTERFACE 
        "${CMAKE_CURRENT_SOURCE_DIR}/win64/lua51.lib"
    )
    target_include_directories(luajit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/win64/)

    add_executable(luajit::cli IMPORTED GLOBAL)
    set_target_properties(luajit::cli PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/win64/luajit.exe"
    )

    function (link_luajit tgt)
        target_link_libraries(${tgt} PRIVATE luajit)
        target_include_directories(${tgt} PRIVATE ${LUAJIT_INCLUDE})
    endfunction()
elseif (UNIX)

    add_library(luajit STATIC IMPORTED GLOBAL)
    set_target_properties(luajit PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/osx-arm64/libluajit.a"
    )

    add_executable(luajit::cli IMPORTED GLOBAL)
    set_target_properties(luajit::cli PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/osx-arm64/luajit"
    )

    target_include_directories(luajit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/osx-arm64/)
endif()
