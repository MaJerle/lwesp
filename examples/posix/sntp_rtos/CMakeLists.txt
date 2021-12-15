cmake_minimum_required(VERSION 3.10)

project(lwesp_sntp_rtos)

set(C_SOURCES
    "../../../snippets/sntp.c"
    "../../../snippets/station_manager.c"
    "../../../snippets/utils.c"
    "../../../lwesp/src/lwesp/lwesp_ap.c"
    "../../../lwesp/src/lwesp/lwesp_buff.c"
    "../../../lwesp/src/lwesp/lwesp_conn.c"
    "../../../lwesp/src/lwesp/lwesp_debug.c"
    "../../../lwesp/src/lwesp/lwesp_dns.c"
    "../../../lwesp/src/lwesp/lwesp_evt.c"
    "../../../lwesp/src/lwesp/lwesp_input.c"
    "../../../lwesp/src/lwesp/lwesp_int.c"
    "../../../lwesp/src/lwesp/lwesp_mem.c"
    "../../../lwesp/src/lwesp/lwesp_parser.c"
    "../../../lwesp/src/lwesp/lwesp_pbuf.c"
    "../../../lwesp/src/lwesp/lwesp_sntp.c"
    "../../../lwesp/src/lwesp/lwesp_sta.c"
    "../../../lwesp/src/lwesp/lwesp_threads.c"
    "../../../lwesp/src/lwesp/lwesp_timeout.c"
    "../../../lwesp/src/lwesp/lwesp_unicode.c"
    "../../../lwesp/src/lwesp/lwesp_wps.c"
    "../../../lwesp/src/lwesp/lwesp_utils.c"
    "../../../lwesp/src/lwesp/lwesp.c"
    "../../../lwesp/src/system/lwesp_ll_posix.c"
    "../../../lwesp/src/system/lwesp_mem_posix.c"
    "../../../lwesp/src/system/lwesp_sys_posix.c"
    "main.c"
)

set(C_INCLUDES
    "."
    "../../../lwesp/src/include/system/port/posix"
    "../../../lwesp/src/include"
    "../../../snippets/include"
)

set(C_LIBRARIES
    "pthread"
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

add_executable(${PROJECT_NAME} ${C_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${C_INCLUDES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${C_LIBRARIES})
