# 
# LIB_PREFIX: LWESP
#
# This file provides set of variables for end user
# and also generates one (or more) libraries, that can be added to the project using target_link_libraries(...)
#
# Before this file is included to the root CMakeLists file (using include() function), user can set some variables:
#
# LWESP_SYS_PORT: If defined, it will include port source file from the library, and include the necessary header file.
# LWESP_OPTS_FILE: If defined, it is the path to the user options file. If not defined, one will be generated for you automatically
# LWESP_COMPILE_OPTIONS: If defined, it provide compiler options for generated library.
# LWESP_COMPILE_DEFINITIONS: If defined, it provides "-D" definitions to the library build
#

# Custom include directory
set(LWESP_CUSTOM_INC_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib_inc)

# Library core sources
set(lwesp_core_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_ap.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_ble.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_bt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_buff.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_cli.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_conn.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_dhcp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_dns.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_evt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_flash.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_hostname.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_input.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_int.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_mdns.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_mem.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_parser.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_pbuf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_ping.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_server.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_smart.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_sntp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_sta.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_threads.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_timeout.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_unicode.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_webserver.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_wps.c
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp.c
)

# API sources
set(lwesp_api_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/api/lwesp_netconn.c
)

# HTTP server
set(lwesp_httpsrv_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/http_server/lwesp_http_server.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/http_server/lwesp_http_server_fs.c
)

# MQTT
set(lwesp_mqtt_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/mqtt/lwesp_mqtt_client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/mqtt/lwesp_mqtt_client_api.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/cayenne/lwesp_cayenne.c
    ${CMAKE_CURRENT_LIST_DIR}/src/apps/cayenne/lwesp_cayenne_evt.c
)

# CLI
set(lwesp_cli_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/lwesp/lwesp_cli.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cli/cli.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cli/cli_input.c
)

# All apps source files
set(lwesp_allapps_SRCS
    ${lwesp_mqtt_SRCS}
    ${lwesp_httpsrv_SRCS}
    ${lwesp_cli_SRCS}
)

# Setup include directories
set(lwesp_include_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/src/include
    ${LWESP_CUSTOM_INC_DIR}
)

# Add system port to core if user defined
if(DEFINED LWESP_SYS_PORT)
    set(lwesp_core_SRCS ${lwesp_core_SRCS} ${CMAKE_CURRENT_LIST_DIR}/src/system/lwesp_sys_${LWESP_SYS_PORT}.c)
    set(lwesp_include_DIRS ${lwesp_include_DIRS} ${CMAKE_CURRENT_LIST_DIR}/src/include/system/port/${LWESP_SYS_PORT})
endif()

# Register core library to the system
add_library(lwesp INTERFACE)
target_sources(lwesp PUBLIC ${lwesp_core_SRCS})
target_include_directories(lwesp INTERFACE ${lwesp_include_DIRS})
target_compile_options(lwesp PRIVATE ${LWESP_COMPILE_OPTIONS})
target_compile_definitions(lwesp PRIVATE ${LWESP_COMPILE_DEFINITIONS})

# Register API to the system
add_library(lwesp_api INTERFACE)
target_sources(lwesp_api PUBLIC ${lwesp_api_SRCS})
target_include_directories(lwesp_api INTERFACE ${lwesp_include_DIRS})
target_compile_options(lwesp_api PRIVATE ${LWESP_COMPILE_OPTIONS})
target_compile_definitions(lwesp_api PRIVATE ${LWESP_COMPILE_DEFINITIONS})

# Register apps to the system
add_library(lwesp_apps INTERFACE)
target_sources(lwesp_apps PUBLIC ${lwesp_allapps_SRCS})
target_include_directories(lwesp_apps INTERFACE ${lwesp_include_DIRS})
target_compile_options(lwesp_apps PRIVATE ${LWESP_COMPILE_OPTIONS})
target_compile_definitions(lwesp_apps PRIVATE ${LWESP_COMPILE_DEFINITIONS})

# Create config file if user didn't provide one info himself
if(NOT LWESP_OPTS_FILE)
    message(STATUS "Using default lwesp_opts.h file")
    set(LWESP_OPTS_FILE ${CMAKE_CURRENT_LIST_DIR}/src/include/lwesp/lwesp_opts_template.h)
else()
    message(STATUS "Using custom lwesp_opts.h file from ${LWESP_OPTS_FILE}")
endif()
configure_file(${LWESP_OPTS_FILE} ${LWESP_CUSTOM_INC_DIR}/lwesp_opts.h COPYONLY)
