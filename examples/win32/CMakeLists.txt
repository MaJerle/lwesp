cmake_minimum_required(VERSION 3.22)

# Setup project
project(${PROJECT_NAME})
add_executable(${PROJECT_NAME})
message("Project name: ${PROJECT_NAME}")

# Add source files
target_sources(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}/main.c

    # Port
    ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/system/lwesp_sys_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/system/lwesp_ll_win32.c

    # Snippets
    ${CMAKE_CURRENT_LIST_DIR}/../../snippets/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/../../snippets/station_manager.c
    ${CMAKE_CURRENT_LIST_DIR}/../../snippets/examples_common_lwesp_callback_func.c
)

# Add include paths
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}/

    # Port
    ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/include/system/port/win32

    # Snippets
    ${CMAKE_CURRENT_LIST_DIR}/../../snippets/include
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
add_subdirectory("../../lwesp" lwesp)
target_link_libraries(${PROJECT_NAME} lwesp)

# Project specific sources and libs
if (${PROJECT_NAME} STREQUAL "client_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/client.c)
endif()
if (${PROJECT_NAME} STREQUAL "dns_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/dns.c)
endif()
if (${PROJECT_NAME} STREQUAL "mqtt_client_api_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/apps/mqtt/lwesp_mqtt_client.c)
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/apps/mqtt/lwesp_mqtt_client_api.c)
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/mqtt_client.c)
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/mqtt_client_api.c)
target_link_libraries(${PROJECT_NAME}   lwesp_api)
endif()
if (${PROJECT_NAME} STREQUAL "mqtt_client_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/apps/mqtt/lwesp_mqtt_client.c)
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../lwesp/src/apps/mqtt/lwesp_mqtt_client_api.c)
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/mqtt_client.c)
endif()
if (${PROJECT_NAME} STREQUAL "netconn_client_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/netconn_client.c)
target_link_libraries(${PROJECT_NAME}   lwesp_api)
endif()
if (${PROJECT_NAME} STREQUAL "netconn_server_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/netconn_server.c)
target_link_libraries(${PROJECT_NAME}   lwesp_api)
endif()
if (${PROJECT_NAME} STREQUAL "sntp_rtos")
target_sources(${PROJECT_NAME} PUBLIC   ${CMAKE_CURRENT_LIST_DIR}/../../snippets/sntp.c)
endif()
