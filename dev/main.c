// lwesp_dev_os.cpp : Defines the entry point for the console application.
//

#include <string.h>
#include "examples_common_lwesp_callback_func.h"
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/apps/lwesp_mqtt_client_api.h"
#include "lwesp/lwesp.h"
#include "windows.h"

#include "cayenne.h"
#include "http_server.h"
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/lwesp_opt.h"
#include "lwesp/lwesp_timeout.h"
#include "lwmem/lwmem.h"
#include "mqtt_client.h"
#include "mqtt_client_api.h"
#include "netconn_client.h"
#include "netconn_server.h"
#include "netconn_server_1thread.h"
#include "sntp.h"
#include "station_manager.h"
#include "utils.h"

#define safeprintf printf

static void main_thread(void* arg);
static void input_thread(void* arg);

static lwesp_ap_t aps[10];
static size_t aps_count;

static lwespr_t lwesp_evt(lwesp_evt_t* evt);

static lwesp_sta_info_ap_t connected_ap_info;
extern volatile uint8_t lwesp_ll_win32_driver_ignore_data;
static lwesp_cayenne_t cayenne;

static uint8_t parse_str(char** str, char** out);
static uint8_t parse_num_u64(char** str, uint64_t* out);
static uint8_t parse_num(char** str, uint32_t* out);

/**
 * \brief           LwMEM memory config
 */
uint8_t lwmem_region_1[0x4000];
lwmem_region_t lwmem_regions[] = {
    {lwmem_region_1, sizeof(lwmem_region_1)},
    {NULL, 0},
};

/**
 * \brief           Command structure
 */
typedef struct {
    uint8_t del;             /*!< Delimiter */
    const char* cmd;         /*!< Command text */
    const char* help_params; /*!< Help parameters */
    const char* help_text;   /*!< Help long text */
} cmd_t;

/**
 * \brief           List of test commands
 */
static const cmd_t cmd_commands[] = {
    {0, "help", "", "Print help for commands"},
    {0, "join", "<ssid> [<pwd> [<mac>]]", "Join to access point"},
    {0, "reconn_set", "<interval> <repeat>", "Set reconnect config"},
    {0, "quit", "", "Quit from access point"},
    {1, "IP management", NULL, NULL},
    {0, "stagetip", "", "Get station IP address"},
    {0, "stasetip", "<ip>", "Set station IP address"},
    {0, "apgetip", "", "Get Soft Access point IP address"},
    {0, "apsetip", "<ip>", "Set Soft Access point IP address"},
    {0, "setdhcp", "<enable>", "Enable or disable DHCP"},
    {1, "MAC management", NULL, NULL},
    {0, "stagetmac", "", "Get station MAC address"},
    {0, "stasetmac", "<mac>", "Set station MAC address"},
    {0, "apgetmac", "", "Get Soft Access point MAC address"},
    {0, "apsetmac", "<mac>", "Set Soft Access point MAC address"},
    {1, "Access point", NULL, NULL},
    {0, "apconfig", "<enable> [<ssid> <pass> <enc> <ch>]", "Configure Soft Access point"},
    {0, "apliststa", "", "List stations connected to access point"},
    {0, "apquitsta", "<mac>", "Disconnect station for Soft access point"},
    {1, "Hostname", NULL, NULL},
    {0, "hnset", "<hostname>", "Set station hostname"},
    {0, "hnget", "", "Get station hostname"},
    {1, "Misc", NULL, NULL},
    {0, "ping", "<host>", "Ping domain or IP address"},
    {1, "Separate threads", NULL, NULL},
    {0, "netconn_client", "", "Start netconn client thread"},
    {0, "netconn_server", "", "Start netconn server thread"},
    {0, "mqtt_client_api", "", "Start mqtt client API thread"},
    {0, "ciupdate", "", "Run ciupdate command"},
    {0, "dhcpenable", "", "Enable DHCP"},
    {0, "dhcpdisable", "", "Disable DHCP"},
    {0, "wifidisable", "", "Disable WIFI mode completely"}};

/**
 * \brief           Program entry point
 */
int
main() {
    safeprintf("App start!\r\n");

    /* Setup regions for memory as a start */
    if (!lwmem_assignmem(lwmem_regions)) {
        safeprintf("Could not assign memory for LwMEM!\r\n");
        return -1;
    }

    /* Create main threads */
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main_thread, NULL, 0, NULL);
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)input_thread, NULL, 0, NULL);

    /* Do nothing at this point but do not close the program */
    while (1) {
        lwesp_delay(1000);
    }
}

/**
 * \brief           Main thread for init purposes
 */
static void
main_thread(void* arg) {
    char hn[10];
    uint32_t ping_time;

    LWESP_UNUSED(hn);
    LWESP_UNUSED(arg);
    LWESP_UNUSED(ping_time);

    /* Init ESP library */
    lwesp_init(lwesp_evt, 1);
    if (0) {
#if LWESP_CFG_ESP32
    } else if (lwesp_device_is_device(LWESP_DEVICE_ESP32)) {
        safeprintf("Device is ESP32\r\n");
#endif /* LWESP_CFG_ESP32 */
#if LWESP_CFG_ESP8266
    } else if (lwesp_device_is_device(LWESP_DEVICE_ESP8266)) {
        safeprintf("Device is ESP8266\r\n");
#endif /* LWESP_CFG_ESP8266 */
#if LWESP_CFG_ESP32_C2
    } else if (lwesp_device_is_device(LWESP_DEVICE_ESP32_C2)) {
        safeprintf("Device is ESP32-C2\r\n");
#endif /* LWESP_CFG_ESP32_C2 */
#if LWESP_CFG_ESP32_C3
    } else if (lwesp_device_is_device(LWESP_DEVICE_ESP32_C3)) {
        safeprintf("Device is ESP32-C3\r\n");
#endif /* LWESP_CFG_ESP32_C3 */
#if LWESP_CFG_ESP32_C6
    } else if (lwesp_device_is_device(LWESP_DEVICE_ESP32_C6)) {
        safeprintf("Device is ESP32-C6\r\n");
#endif /* LWESP_CFG_ESP32_C6 */
    } else {
        safeprintf("Unknown device...\r\n");
    }

#if 0
    uint8_t read_data_buffer[1024];
    uint32_t read_data_length;
    lwesp_mfg_read(LWESP_MFG_NAMESPACE_SERVER_CERT, "server_cert", read_data_buffer, sizeof(read_data_buffer), 0,
                   &read_data_length, NULL, NULL, 1);
    printf("Len: %u\r\n", (unsigned)read_data_length);
    lwesp_mfg_read(LWESP_MFG_NAMESPACE_SERVER_CERT, "server_cert", read_data_buffer, 15, 20, &read_data_length, NULL,
                   NULL, 1);
    printf("Len: %u\r\n", (unsigned)read_data_length);
#endif

    /* Start thread to toggle device present */
    //lwesp_sys_thread_create(NULL, "device_present", (lwesp_sys_thread_fn)lwesp_device_present_toggle, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /*
     * Try to connect to preferred access point
     *
     * Follow function implementation for more info
     * on how to setup preferred access points for fast connection
     */
    lwesp_sta_autojoin(0, NULL, NULL, 1);

    /*
     * Initialize and start asynchronous connection to preferred acces point
     *
     * Will immediately return and will not block the application.
     * All events are done asynchronously
     */
    station_manager_connect_to_access_point_async_init();

    /* Different types of snippets to execute */

    /* HTTP server application example */

    /* SNTP example */
    sntp_gettime();

    /* Netconn client in separate thread */
    //lwesp_sys_thread_create(NULL, "netconn_client", (lwesp_sys_thread_fn)netconn_client_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* Netconn client in separate thread */
    lwesp_sys_thread_create(NULL, "netconn_client_ssl", (lwesp_sys_thread_fn)netconn_client_ssl_thread, NULL, 0,
                            LWESP_SYS_THREAD_PRIO);

    /* Netconn server with multiple threads */
    //lwesp_sys_thread_create(NULL, "netconn_server", (lwesp_sys_thread_fn)netconn_server_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* Netconn server with single thread */
    //lwesp_sys_thread_create(NULL, "netconn_server_single", (lwesp_sys_thread_fn)netconn_server_1thread_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* MQTT client with asynchronous events */
    //lwesp_sys_thread_create(NULL, "mqtt_client", (lwesp_sys_thread_fn)mqtt_client_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* MQTT client with API sequential mode, test application */
    //lwesp_sys_thread_create(NULL, "mqtt_client_api", (lwesp_sys_thread_fn)mqtt_client_api_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* MQTT API client with connectivity to Cayenne */
    //lwesp_sys_thread_create(NULL, "mqtt_client_api_cayenne", (lwesp_sys_thread_fn)mqtt_client_api_cayenne_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

    /* LwESP built-in Cayenne protocol implementation thread demo */
    //lwesp_sys_thread_create(NULL, "cayenne", (lwesp_sys_thread_fn)cayenne_thread, NULL, 0, LWESP_SYS_THREAD_PRIO);

#if 0
    /* Asynchronous push-only cayenne demo with buffer used to fill the data */
    cayenne_async_mqtt_init();

    /* Demo to fill cayenne async data */
    while (1) {
        static cayenne_async_data_t data;
        static float temp = 0.03;
        static uint32_t channel = 103;

        /* Setup value */
        data.type = CAYENNE_DATA_TYPE_TEMP;
        data.channel = channel;
        data.data.flt = temp;
        if (lwesp_buff_get_free(&cayenne_async_data_buff) > sizeof(data)) {
            lwesp_buff_write(&cayenne_async_data_buff, &data, sizeof(data));
        }

        /* Set new value for next round */
        if (channel++ > 113) {
            channel = 103;
        }
        temp *= 1.01;
        lwesp_delay(3000);
    }
#endif

    /* While loop with delay to prevent main thread termination in development environment */
    while (1) {
        lwesp_delay(1000);
    }
}

/**
 * \brief           Console input thread
 */
static void
input_thread(void* arg) {
    char buff[128];
    char* str;
    const cmd_t* cmd;

#define IS_LINE(s) (strncmp(buff, (s), sizeof(s) - 1) == 0)

    LWESP_UNUSED(arg);

    /* Notify user */
    safeprintf("Start by writing commands..\r\n");

    /* Very simple input */
    while (1) {
        safeprintf(" > ");
        memset(buff, 0x00, sizeof(buff));
        fgets(buff, sizeof(buff), stdin);

        /* Analyze input data */
        size_t i = 0;
        for (i = 0; i < LWESP_ARRAYSIZE(cmd_commands); ++i) {
            if (cmd_commands[i].del) {
                continue;
            }
            if (!strncmp(buff, cmd_commands[i].cmd, strlen(cmd_commands[i].cmd))) {
                cmd = &cmd_commands[i];
                break;
            }
        }
        if (i == LWESP_ARRAYSIZE(cmd_commands)) {
            safeprintf("[CMD] Unknown input command\r\n");
            continue;
        }
        safeprintf("cmd name: %s\r\n", cmd->cmd);
        str = buff + strlen(cmd->cmd);

        /* Process each command */
        if (IS_LINE("join")) {
            char *ssid, *pass;

            parse_str(&str, &ssid);
            parse_str(&str, &pass);

            lwesp_sta_join(ssid, pass, NULL, NULL, NULL, 1);
        } else if (IS_LINE("quit")) {
            lwesp_sta_quit(NULL, NULL, 1);
        } else if (IS_LINE("reconn_set")) {
            uint32_t interval, rep_cnt = 0;
            parse_num(&str, &interval);
            if (interval > 0) {
                parse_num(&str, &rep_cnt);
            }
            lwesp_sta_reconnect_set_config(interval, rep_cnt, NULL, NULL, 1);
        } else if (IS_LINE("setip")) {
            lwesp_ip_t dev_ip;
            lwesp_ip_set_ip4(&dev_ip, 192, 168, 1, 150);
            lwesp_sta_setip(&dev_ip, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("getip")) {
            lwesp_sta_getip(NULL, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("dhcpenable")) {
            lwesp_dhcp_set_config(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("dhcpdisable")) {
            lwesp_dhcp_set_config(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("listap")) {
            lwesp_sta_list_ap(NULL, aps, LWESP_ARRAYSIZE(aps), &aps_count, NULL, NULL, 1);
            safeprintf("Detected %d number of access points\r\n", (int)aps_count);
        } else if (IS_LINE("getapinfo")) {
            lwesp_sta_info_ap_t ap;
            lwesp_sta_get_ap_info(&ap, NULL, NULL, 1);
        } else if (IS_LINE("apenable")) {
            lwesp_set_wifi_mode(LWESP_MODE_STA_AP, NULL, NULL, 1);
            lwesp_ap_set_config("ESP8266_SSID", "its private", 13, LWESP_ECN_WPA2_PSK, 5, 0, NULL, NULL, 1);
        } else if (IS_LINE("apdisable")) {
            lwesp_set_wifi_mode(LWESP_MODE_STA, NULL, NULL, 1);
        } else if (IS_LINE("wifidisable")) {
            lwesp_set_wifi_mode(LWESP_MODE_NONE, NULL, NULL, 1);
        } else if (IS_LINE("apliststa")) {
            lwesp_sta_t stas[10];
            size_t stat;

            lwesp_ap_list_sta(stas, LWESP_ARRAYSIZE(stas), &stat, NULL, NULL, 1);
            safeprintf("Number of stations: %d\r\n", (int)stat);
        } else if (IS_LINE("ping")) {
            uint32_t pingtime;
            char* host;

            if (parse_str(&str, &host)) {
                if (lwesp_ping(host, &pingtime, NULL, NULL, 1) == lwespOK) {
                    safeprintf("Ping time: %d\r\n", (int)pingtime);
                } else {
                    safeprintf("Error with ping to host \"%s\"\r\n", host);
                }
            } else {
                safeprintf("Cannot parse host\r\n");
            }
        } else if (IS_LINE("hnset")) {
            char* host;

            if (parse_str(&str, &host)) {
                lwesp_hostname_set(host, NULL, NULL, 1);
            } else {
                safeprintf("Cannot parse host\r\n");
            }
        } else if (IS_LINE("hnget")) {
            char hn[20];
            lwesp_hostname_get(hn, sizeof(hn), NULL, NULL, 1);
            safeprintf("Hostname: %s\r\n", hn);
        } else if (IS_LINE("netconn_client")) {
            lwesp_sys_sem_t sem;
            lwesp_sys_sem_create(&sem, 0);
            lwesp_sys_thread_create(NULL, "netconn_client", (lwesp_sys_thread_fn)netconn_client_thread, &sem, 0,
                                    LWESP_SYS_THREAD_PRIO);
            lwesp_sys_sem_wait(&sem, 0);
            lwesp_sys_sem_delete(&sem);
        } else if (IS_LINE("netconn_server")) {
            lwesp_sys_thread_create(NULL, "netconn_server", (lwesp_sys_thread_fn)netconn_server_thread, NULL, 0,
                                    LWESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("mqttthread")) {
            lwesp_sys_thread_create(NULL, "mqtt_client_api", (lwesp_sys_thread_fn)mqtt_client_api_thread, NULL, 0,
                                    LWESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("ignoreon")) {
            safeprintf("Ignoring data...\r\n");
            lwesp_ll_win32_driver_ignore_data = 1;
        } else if (IS_LINE("ignoreoff")) {
            safeprintf("Not ignoring data...\r\n");
            lwesp_ll_win32_driver_ignore_data = 0;
        } else if (IS_LINE("ciupdate")) {
            lwesp_update_sw(NULL, NULL, 1);
        } else {
            safeprintf("Unknown input!\r\n");
        }
    }
}

/**
 * \brief           Global ESP event function callback
 * \param[in]       evt: Event information
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
lwesp_evt(lwesp_evt_t* evt) {
    switch (evt->type) {
        case LWESP_EVT_INIT_FINISH: {
            /* Device is not present on init */
            //lwesp_device_set_present(0, NULL, NULL, 0);
            break;
        }
        case LWESP_EVT_RESET: {
            if (lwesp_evt_reset_get_result(evt) == lwespOK) {
                safeprintf("Reset sequence successful!\r\n");
            } else {
                safeprintf("Reset sequence error!\r\n");
            }
            break;
        }
        case LWESP_EVT_RESTORE: {
            if (lwesp_evt_restore_get_result(evt) == lwespOK) {
                safeprintf("Restore sequence successful!\r\n");
            } else {
                safeprintf("Restore sequence error!\r\n");
            }
            break;
        }
        case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            lwesp_sw_version_t v_min, v_curr;

            lwesp_get_min_at_fw_version(&v_min);
            lwesp_get_current_at_fw_version(&v_curr);

            safeprintf("Current ESP[8266/32[-C3]] AT version is not supported by the library\r\n");
            safeprintf("Minimum required AT version is: %08X\r\n", (unsigned)v_min.version);
            safeprintf("Current AT version is: %08X\r\n", (unsigned)v_curr.version);
            break;
        }
        case LWESP_EVT_WIFI_GOT_IP: {
            safeprintf("Wifi got an IP address.\r\n");
            if (lwesp_sta_has_ipv6_local()) {
                safeprintf("Wifi got IPv6 local IP address.\r\n");
            }
            if (lwesp_sta_has_ipv6_global()) {
                safeprintf("Wifi got IPv6 global IP address.\r\n");
            }
            break;
        }
        case LWESP_EVT_WIFI_CONNECTED: {
            safeprintf("Wifi just connected. Read access point information\r\n");
            lwesp_sta_get_ap_info(&connected_ap_info, NULL, NULL, 0);
            break;
        }
        case LWESP_EVT_WIFI_DISCONNECTED: {
            safeprintf("Wifi just disconnected\r\n");
            break;
        }
        case LWESP_EVT_STA_INFO_AP: {
            safeprintf("SSID: %s, ch: %d, rssi: %d\r\n", lwesp_evt_sta_info_ap_get_ssid(evt),
                       (int)lwesp_evt_sta_info_ap_get_channel(evt), (int)lwesp_evt_sta_info_ap_get_rssi(evt));
            break;
        }
        case LWESP_EVT_WIFI_IP_ACQUIRED: {
            lwesp_ip_t ip;
            uint8_t is_dhcp;

            safeprintf("WIFI IP ACQUIRED!\r\n");
            if (lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp) == lwespOK) {
                utils_print_ip("IP: ", &ip, "\r\n");
            } else {
                safeprintf("Acquired IP is not valid\r\n");
            }
            break;
        }
#if LWESP_CFG_MODE_ACCESS_POINT
        case LWESP_EVT_AP_CONNECTED_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_connected_sta_get_mac(evt);
            utils_print_mac("New station connected to AP with MAC: ", mac, "\r\n");
            break;
        }
        case LWESP_EVT_AP_DISCONNECTED_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_disconnected_sta_get_mac(evt);
            utils_print_mac("New station disconnected from AP with MAC: ", mac, "\r\n");
            break;
        }
        case LWESP_EVT_AP_IP_STA: {
            lwesp_mac_t* mac = lwesp_evt_ap_ip_sta_get_mac(evt);
            lwesp_ip_t* ip = lwesp_evt_ap_ip_sta_get_ip(evt);
            utils_print_ip("Station got IP address (from AP): ", ip, "");
            utils_print_mac(" and MAC: ", mac, "\r\n");
            break;
        }
#endif /* LWESP_CFG_MODE_ACCESS_POINT */
        default: break;
    }
    return lwespOK;
}

/**
 * \brief           Parse string and move pointer after parse
 * \param[in,out]   str: Pointer to pointer to input string
 * \param[out]      out: Output variable to set beg of pointer
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
parse_str(char** str, char** out) {
    char* s = *str;
    uint8_t is_quote = 0;

    *out = NULL;
    for (; s != NULL && *s != '\0' && *s == ' '; ++s) {}
    if (s != NULL && *s >= ' ') {
        if (*s == '"') {
            is_quote = 1;
            ++s;
        } else if (*s == '\0') {
            return 0;
        }
        *out = s; /* Set where we point */
        for (; s != NULL && *s >= ' ' && *s != (is_quote ? '"' : ' '); ++s) {}
        *s = '\0';
        *str = s + 1; /* Set new value for str */
        return 1;
    } else {
        *out = NULL;
        return 0;
    }
}

/**
 * \brief           Parse number in dec, oct, hex or bin format
 * \param[in,out]   str: Pointer to pointer to input string
 * \param[out]      out: Output variable to write value
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
parse_num_u64(char** str, uint64_t* out) {
    uint64_t r, num = 0;
    char* s = *str;
    unsigned char c;

    *out = 0;
    for (; s != NULL && *s != '\0' && *s == ' '; ++s) {}
    if (s != NULL && *s >= '0' && *s <= '9') {
        /* Check for hex/bin/octal */
        if (*s == '0') {
            ++s;
            if (*s == 'x' || *s == 'X') {
                r = 16;
                ++s;
            } else if (*s == 'b' || *s == 'B') {
                r = 2;
                ++s;
            } else if (*s <= '7') {
                r = 8;
            } else if (*s <= ' ') {
                return 1; /* Single zero */
            } else {
                return 0; /* Wrong format */
            }
        } else {
            r = 10;
        }

        num = 0;
        for (c = *s; c > ' '; ++s, c = *s) {
            if (c > 'a') {
                c -= 0x20;
            }
            c -= '0';
            if (c > 17) {
                c -= 7;
                if (c <= 9) {
                    return 0;
                }
            }
            if (c >= r) {
                return 0;
            }
            num = num * r + c;
        }
        *out = num;
        *str = s;
        return 1;
    }
    return 0;
}

/**
 * \brief           Parse number in dec, oct, hex or bin format
 * \param[in,out]   str: Pointer to pointer to input string
 * \param[out]      out: Output variable to write value
 * \return          `1` on success, `0` otherwise
 */
static uint8_t
parse_num(char** str, uint32_t* out) {
    uint64_t num;
    uint8_t s;

    s = parse_num_u64(str, &num);
    *out = (uint32_t)num;

    return s;
}
