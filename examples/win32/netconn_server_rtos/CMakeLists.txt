cmake_minimum_required(VERSION 3.22)

# Setup project
project(netconn_server_rtos)
add_executable(${PROJECT_NAME})

# Add key executable block
target_sources(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c

    # Port
    ${CMAKE_CURRENT_LIST_DIR}/../../../lwesp/src/system/lwesp_sys_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../lwesp/src/system/lwesp_ll_win32.c

    # Snippets
    ${CMAKE_CURRENT_LIST_DIR}/../../../snippets/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../snippets/station_manager.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../snippets/examples_common_lwesp_callback_func.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../snippets/netconn_server.c
)

# Add key include paths
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/

    # Port
    ${CMAKE_CURRENT_LIST_DIR}/../../../lwesp/src/include/system/port/win32

    # Snippets
    ${CMAKE_CURRENT_LIST_DIR}/../../../snippets/include
)

# Compilation definition information
target_compile_definitions(${PROJECT_NAME} PUBLIC
    WIN32
    _DEBUG
    CONSOLE
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Add subdir with lwesp and link to the project
add_subdirectory("../../../lwesp" lwesp)
target_link_libraries(${PROJECT_NAME} lwesp)
target_link_libraries(${PROJECT_NAME} lwesp_api)
