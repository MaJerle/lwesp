cmake_minimum_required(VERSION 3.13)

# Debug message
message("LWESP: Entering lib source CMakeLists.txt")

# Set library name
set(LWESP_LIB_NAME "lwesp")

# Check definitions of some key things
if(NOT DEFINED LWESP_SYS_ARCH)
    message("LWESP: LWESP_SYS_ARCH not defined. Using default one: \"win32\"")
    set(LWESP_SYS_ARCH "win32")
endif()
if(NOT DEFINED LWESP_LL_ARCH)
    message("LWESP: LWESP_LL_ARCH not defined. Using default one: \"win32\"")
    set(LWESP_LL_ARCH "win32")
endif()

# Register library to the system
add_library(${LWESP_LIB_NAME} INTERFACE)

# Setup generic source files
target_sources(${LWESP_LIB_NAME} PUBLIC
    # Core libraries
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_cli.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_dhcp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_dns.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_hostname.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_ping.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_ap.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_buff.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_conn.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_evt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_input.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_int.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_mdns.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_mem.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_parser.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_pbuf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_sntp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_sta.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_threads.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_timeout.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_unicode.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_wps.c
    
    # API part
    ${CMAKE_CURRENT_LIST_DIR}/src/api/lwesp_netconn.c

    # Applications
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/http_server/lwesp_http_server.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/http_server/lwesp_http_server_fs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/http_server/lwesp_http_server_fs_win32.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/mqtt/lwesp_mqtt_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/cayenne/lwesp_cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/cayenne/lwesp_cayenne_evt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/mqtt/lwesp_mqtt_client_api.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cli/cli.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cli/cli_input.c

    # Port files
    ${CMAKE_CURRENT_LIST_DIR}/src/system/lwesp_sys_${LWESP_SYS_ARCH}.c
    ${CMAKE_CURRENT_LIST_DIR}/src/system/lwesp_ll_${LWESP_SYS_ARCH}.c
    )

# Setup include directories
target_include_directories(${LWESP_LIB_NAME} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
    ${CMAKE_CURRENT_LIST_DIR}/src/include/system/port/${LWESP_SYS_ARCH}
    )
