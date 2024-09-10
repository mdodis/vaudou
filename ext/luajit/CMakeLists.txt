if (WIN32)

    add_library(luajit INTERFACE)

    target_link_libraries(luajit INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win64/luajit.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/win64/lua51.lib"
    )
    
    set(LUAJIT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/win64" CACHE PATH "LuaJIT include directory")

    function (link_luajit tgt)
        target_link_libraries(${tgt} PRIVATE luajit)
        target_include_directories(${tgt} PRIVATE ${LUAJIT_INCLUDE})
    endfunction()
endif()