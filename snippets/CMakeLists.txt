cmake_minimum_required(VERSION 3.13)

# Debug message
message("Entering ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt")

# Register library to the system
add_library(lwesp_snippets INTERFACE)

# Setup generic source files
target_sources(lwesp_snippets PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/telnet_server.c
    ${CMAKE_CURRENT_LIST_DIR}/utils.c
    ${CMAKE_CURRENT_LIST_DIR}/cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/client.c
    ${CMAKE_CURRENT_LIST_DIR}/dns.c
    ${CMAKE_CURRENT_LIST_DIR}/http_server.c
    ${CMAKE_CURRENT_LIST_DIR}/mqtt_client.c
    ${CMAKE_CURRENT_LIST_DIR}/mqtt_client_api.c
    ${CMAKE_CURRENT_LIST_DIR}/mqtt_client_api_cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/netconn_client.c
    ${CMAKE_CURRENT_LIST_DIR}/netconn_server.c
    ${CMAKE_CURRENT_LIST_DIR}/netconn_server_1thread.c
    ${CMAKE_CURRENT_LIST_DIR}/sntp.c
    ${CMAKE_CURRENT_LIST_DIR}/station_manager.c
    )

# Setup include directories
target_include_directories(lwesp_snippets INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
    )

# Debug message
message("Exiting ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt")
