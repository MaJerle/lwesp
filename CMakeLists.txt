cmake_minimum_required(VERSION 3.22)

# Setup project
project(LwLibPROJECT)

if(NOT PROJECT_IS_TOP_LEVEL)
    add_subdirectory(lwesp)
else()
    # Set as executable
    add_executable(${PROJECT_NAME})

    # Add key executable block
    target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/dev/main.c

        # Development additional files
        ${CMAKE_CURRENT_LIST_DIR}/lwesp/src/system/lwesp_mem_lwmem.c
        ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/lwmem/lwmem.c
        ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/system/lwmem_sys_win32.c

        # Http server file system
        ${CMAKE_CURRENT_LIST_DIR}/lwesp/src/apps/http_server/lwesp_http_server_fs_win32.c

        # Port specific
        ${CMAKE_CURRENT_LIST_DIR}/lwesp/src/system/lwesp_ll_win32.c
    )

    # Add key include paths
    target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/dev
        ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/include
    )

    # Compilation definition information
    target_compile_definitions(${PROJECT_NAME} PUBLIC
        WIN32
        _DEBUG
        CONSOLE
        LWESP_DEV
    )

    # Compiler options
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    # Add subdir with lwesp and link to the project
    set(LWESP_OPTS_DIR ${CMAKE_CURRENT_LIST_DIR}/dev)
    set(LWESP_SYS_PORT "win32")
    add_subdirectory(lwesp)
    target_link_libraries(${PROJECT_NAME} lwesp)
    target_link_libraries(${PROJECT_NAME} lwesp_api)
    target_link_libraries(${PROJECT_NAME} lwesp_apps)
    add_subdirectory(snippets)
    target_link_libraries(${PROJECT_NAME} lwesp_snippets)
endif()
