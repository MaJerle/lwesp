cmake_minimum_required(VERSION 3.0.0)

# Setup project
project(LwLibPROJECT)

# -------------------------------------------------
# This CMakeLists.txt is used only if it is a top-level file.
# Purpose of it is to be able to compile project in standalone way only
#
# When library sources are to be included in another project
# user shall use /lwesp/CMakeLists.txt instead
if (NOT PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "This CMakeLists.txt can only be used as top-level. Use /lwesp/CMakeLists.txt for library include purpose")
endif()

# Set as executable
add_executable(${PROJECT_NAME})

# Add key executable block
target_sources(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/dev/VisualStudio/main.c

    # Snippets
    ${CMAKE_CURRENT_LIST_DIR}/snippets/telnet_server.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/client.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/dns.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/http_server.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/mqtt_client.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/mqtt_client_api.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/mqtt_client_api_cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/netconn_client.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/netconn_server.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/netconn_server_1thread.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/sntp.c
    ${CMAKE_CURRENT_LIST_DIR}/snippets/station_manager.c

    # Development additional files
    ${CMAKE_CURRENT_LIST_DIR}/lwesp/src/system/lwesp_mem_lwmem.c
    ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/lwmem/lwmem.c
    ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/system/lwmem_sys_win32.c
    )

# Add key include paths
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/dev/VisualStudio
    ${CMAKE_CURRENT_LIST_DIR}/snippets/include
    ${CMAKE_CURRENT_LIST_DIR}/../lwmem/lwmem/src/include
    )

# Compilation definition information
target_compile_definitions(${PROJECT_NAME} PUBLIC
    WIN32
    _DEBUG
    CONSOLE
    LWESP_DEV
    )

# Add subdir with lwesp and link to the project
add_subdirectory("lwesp")
target_link_libraries(${PROJECT_NAME} lwesp)