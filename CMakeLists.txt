cmake_minimum_required(VERSION 3.0.0)
project(LwLibPROJECT VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(${PROJECT_NAME}
    lwesp/src/lwesp/lwesp_cli.c
    lwesp/src/lwesp/lwesp_dhcp.c
    lwesp/src/lwesp/lwesp_dns.c
    lwesp/src/lwesp/lwesp_hostname.c
    lwesp/src/lwesp/lwesp_ping.c
    lwesp/src/lwesp/lwesp_utils.c
    lwesp/src/lwesp/lwesp.c
    lwesp/src/lwesp/lwesp_ap.c
    lwesp/src/lwesp/lwesp_buff.c
    lwesp/src/lwesp/lwesp_conn.c
    lwesp/src/lwesp/lwesp_debug.c
    lwesp/src/lwesp/lwesp_evt.c
    lwesp/src/lwesp/lwesp_input.c
    lwesp/src/lwesp/lwesp_int.c
    lwesp/src/lwesp/lwesp_mdns.c
    lwesp/src/lwesp/lwesp_mem.c
    lwesp/src/lwesp/lwesp_parser.c
    lwesp/src/lwesp/lwesp_pbuf.c
    lwesp/src/lwesp/lwesp_sntp.c
    lwesp/src/lwesp/lwesp_sta.c
    lwesp/src/lwesp/lwesp_threads.c
    lwesp/src/lwesp/lwesp_timeout.c
    lwesp/src/lwesp/lwesp_unicode.c
    lwesp/src/lwesp/lwesp_wps.c

    lwesp/src/system/lwesp_ll_win32.c
    lwesp/src/system/lwesp_sys_win32.c
    lwesp/src/system/lwesp_mem_lwmem.c

    lwesp/src/api/lwesp_netconn.c
    
    lwesp/src/apps/http_server/lwesp_http_server.c
    lwesp/src/apps/http_server/lwesp_http_server_fs.c
    lwesp/src/apps/http_server/lwesp_http_server_fs_win32.c
    lwesp/src/apps/mqtt/lwesp_mqtt_client.c
    lwesp/src/apps/cayenne/lwesp_cayenne.c
    lwesp/src/apps/cayenne/lwesp_cayenne_evt.c
    lwesp/src/apps/mqtt/lwesp_mqtt_client_api.c
    lwesp/src/cli/cli.c
    lwesp/src/cli/cli_input.c

    ../lwmem/lwmem/src/lwmem/lwmem.c
    ../lwmem/lwmem/src/system/lwmem_sys_win32.c

    snippets/telnet_server.c
    snippets/utils.c
    snippets/cayenne.c
    snippets/client.c
    snippets/dns.c
    snippets/http_server.c
    snippets/mqtt_client.c
    snippets/mqtt_client_api.c
    snippets/mqtt_client_api_cayenne.c
    snippets/netconn_client.c
    snippets/netconn_server.c
    snippets/netconn_server_1thread.c
    snippets/sntp.c
    snippets/station_manager.c

    dev/VisualStudio/main.c
    )

target_include_directories(${PROJECT_NAME} PRIVATE
    dev/VisualStudio
    lwesp/src/include
    lwesp/src/include/system/port/win32
    snippets/include
    ../lwmem/lwmem/src/include
    )

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32
    _DEBUG
    CONSOLE
    LWESP_DEV
    )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
