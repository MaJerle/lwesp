cmake_minimum_required(VERSION 3.22)

# Debug message
message("Entering ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt")


# Setup generic source files
set(lwesp_snippets_SRCS
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
set(lwesp_snippets_include_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
    )

# Register library to the system
add_library(lwesp_snippets INTERFACE)
target_sources(lwesp_snippets PUBLIC ${lwesp_snippets_SRCS})
target_include_directories(lwesp_snippets INTERFACE ${lwesp_snippets_include_DIRS})

# Debug message
message("Exiting ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt")
