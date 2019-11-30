// esp_dev_os.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include "esp/esp.h"
#include "esp/apps/esp_mqtt_client_api.h"
#include "esp/apps/esp_cayenne.h"

#include "mqtt_client.h"
#include "mqtt_client_api.h"
#include "http_server.h"
#include "station_manager.h"
#include "netconn_client.h"
#include "netconn_server.h"
#include "netconn_server_1thread.h"
#include "string.h"
#include "esp/esp_timeout.h"
#include "lwmem/lwmem.h"
#include "esp_config.h"

#define safeprintf              printf

static void main_thread(void* arg);
static void input_thread(void* arg);

esp_ap_t aps[10];
size_t aps_count;

static espr_t esp_evt(esp_evt_t* evt);

esp_sta_info_ap_t connected_ap_info;
extern volatile uint8_t esp_ll_win32_driver_ignore_data;

/**
 * \brief           LwMEM memory config
 */
uint8_t lwmem_region_1[0x4000];
lwmem_region_t lwmem_regions[] = {
    {lwmem_region_1, sizeof(lwmem_region_1)}
};

/**
 * \brief           Command structure
 */
typedef struct {
    uint8_t del;                                /*!< Delimiter */
    const char* cmd;                            /*!< Command text */
    const char* help_params;                    /*!< Help parameters */
    const char* help_text;                      /*!< Help long text */
} cmd_t;

/**
 * \brief           List of test commands
 */
static const cmd_t
cmd_commands[] = {
    { 0, "help", "", "Print help for commands" },
    { 0, "join", "<ssid> [<pwd> [<mac>]]", "Join to access point" },
    { 0, "quit", "", "Quit from access point" },
    { 1, "IP management" },
    { 0, "stagetip", "", "Get station IP address" },
    { 0, "stasetip", "<ip>", "Set station IP address" },
    { 0, "apgetip", "", "Get Soft Access point IP address" },
    { 0, "apsetip", "<ip>", "Set Soft Access point IP address" },
    { 0, "setdhcp", "<enable>", "Enable or disable DHCP" },
    { 1, "MAC management" },
    { 0, "stagetmac", "", "Get station MAC address" },
    { 0, "stasetmac", "<mac>", "Set station MAC address" },
    { 0, "apgetmac", "", "Get Soft Access point MAC address" },
    { 0, "apsetmac", "<mac>", "Set Soft Access point MAC address" },
    { 1, "Access point" },
    { 0, "apconfig", "<enable> [<ssid> <pass> <enc> <ch>]", "Configure Soft Access point" },
    { 0, "apliststa", "", "List stations connected to access point" },
    { 0, "apquitsta", "<mac>", "Disconnect station for Soft access point" },
    { 1, "Hostname" },
    { 0, "hnset", "<hostname>", "Set station hostname" },
    { 0, "hnget", "", "Get station hostname" },
    { 1, "Misc" },
    { 0, "ping", "<host>", "Ping domain or IP address"},
    { 1, "Separate threads" },
    { 0, "netconn_client", "", "Start netconn client thread"},
    { 0, "netconn_server", "", "Start netconn server thread"},
    { 0, "mqtt_client_api", "", "Start mqtt client API thread"},
};

/**
 * \brief           Program entry point
 */
int
main() {
    safeprintf("App start!\r\n");

    if (!lwmem_assignmem(lwmem_regions, sizeof(lwmem_regions) / sizeof(lwmem_regions[0]))) {
        safeprintf("Could not assign memory for LwMEM!\r\n");
        return -1;
    }

    /* Create main thread */
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main_thread, NULL, 0, NULL);
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)input_thread, NULL, 0, NULL);

    /* Do nothing at this point but do not close the program */
    while (1) {
        esp_delay(1000);
    }
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
        *out = s;                               /* Set where we point */
        for (; s != NULL && *s >= ' ' && *s != (is_quote ? '"' : ' '); ++s) {}
        *s = '\0';
        *str = s + 1;                           /* Set new value for str */
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
    char c;
    uint8_t is_quote = 0;

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
                return 1;                       /* Single zero */
            } else {
                return 0;                       /* Wrong format */
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

/**
 * \brief           Console input thread
 */
static void
input_thread(void* arg) {
    char buff[128];
    char* str;
    const cmd_t* cmd;

#define IS_LINE(s)      (strncmp(buff, (s), sizeof(s) - 1) == 0)

    /* Notify user */
    safeprintf("Start by writing commands..\r\n");

    /* Very simple input */
    while (1) {
        safeprintf(" > ");
        memset(buff, 0x00, sizeof(buff));
        fgets(buff, sizeof(buff), stdin);

        /* Analyze input data */
        size_t i = 0;
        for (i = 0; i < ESP_ARRAYSIZE(cmd_commands); ++i) {
            if (cmd_commands[i].del) {
                continue;
            }
            if (!strncmp(buff, cmd_commands[i].cmd, strlen(cmd_commands[i].cmd))) {
                cmd = &cmd_commands[i];
                break;
            }
        }
        if (i == ESP_ARRAYSIZE(cmd_commands)) {
            safeprintf("[CMD] Unknown input command\r\n");
            continue;
        }
        safeprintf("cmd name: %s\r\n", cmd->cmd);
        str = buff + strlen(cmd->cmd);

        /* Process each command */
        if (IS_LINE("join")) {
            char* ssid, *pass;

            parse_str(&str, &ssid);
            parse_str(&str, &pass);

            esp_sta_join(ssid, pass, NULL, NULL, NULL, 1);
        } else if (IS_LINE("quit")) {
            esp_sta_quit(NULL, NULL, 1);
        } else if (IS_LINE("setip")) {
            esp_ip_t dev_ip;
            dev_ip.ip[0] = 192;
            dev_ip.ip[1] = 168;
            dev_ip.ip[2] = 1;
            dev_ip.ip[3] = 150;
            esp_sta_setip(&dev_ip, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("getip")) {
            esp_sta_getip(NULL, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("dhcpenable")) {
            esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("dhcpdisable")) {
            esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("listap")) {
            esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &aps_count, NULL, NULL, 1);
            safeprintf("Detected %d number of access points\r\n", (int)aps_count);
        } else if (IS_LINE("getapinfo")) {
            esp_sta_info_ap_t ap;
            esp_sta_get_ap_info(&ap, NULL, NULL, 1);
        } else if (IS_LINE("apenable")) {
            esp_set_wifi_mode(ESP_MODE_STA_AP, NULL, NULL, 1);
            esp_ap_configure("ESP8266_SSID", "its private", 13, ESP_ECN_WPA2_PSK, 5, 0, NULL, NULL, 1);
        } else if (IS_LINE("apdisable")) {
            esp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 1);
        } else if (IS_LINE("apliststa")) {
            esp_sta_t stas[10];
            size_t stat;

            esp_ap_list_sta(stas, ESP_ARRAYSIZE(stas), &stat, NULL, NULL, 1);
            safeprintf("Number of stations: %d\r\n", (int)stat);
        } else if (IS_LINE("ping")) {
            uint32_t pingtime;
            char* host;
            
            if (parse_str(&str, &host)) {
                esp_ping(host, &pingtime, NULL, NULL, 1);
                safeprintf("Ping time: %d\r\n", (int)pingtime);
            } else {
                safeprintf("Cannot parse host\r\n");
            }
        } else if (IS_LINE("hnset")) {
            char* host;
            
            if (parse_str(&str, &host)) {
                esp_hostname_set(host, NULL, NULL, 1);
            } else {
                safeprintf("Cannot parse host\r\n");
            }
        } else if (IS_LINE("hnget")) {
            char hn[20];
            esp_hostname_get(hn, sizeof(hn), NULL, NULL, 1);
            safeprintf("Hostname: %s\r\n", hn);
        } else if (IS_LINE("netconn_client")) {
            esp_sys_sem_t sem;
            esp_sys_sem_create(&sem, 0);
            esp_sys_thread_create(NULL, "netconn_client", (esp_sys_thread_fn)netconn_client_thread, &sem, 0, ESP_SYS_THREAD_PRIO);
            esp_sys_sem_wait(&sem, 0);
            esp_sys_sem_delete(&sem);
        } else if (IS_LINE("netconn_server")) {
            esp_sys_thread_create(NULL, "netconn_server", (esp_sys_thread_fn)netconn_server_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("mqttthread")) {
            esp_sys_thread_create(NULL, "mqtt_client_api", (esp_sys_thread_fn)mqtt_client_api_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("ignoreon")) {
            safeprintf("Ignoring data...\r\n");
            esp_ll_win32_driver_ignore_data = 1;
        } else if (IS_LINE("ignoreoff")) {
            safeprintf("Not ignoring data...\r\n");
            esp_ll_win32_driver_ignore_data = 0;
        } else {
            safeprintf("Unknown input!\r\n");
        }
    }
}

/**
 * \brief           Main thread for init purposes
 */
static void
main_thread(void* arg) {
    char hn[10];
    uint32_t ping_time;

    /* Init ESP library */
    esp_init(esp_evt, 1);

    if (esp_device_is_esp32()) {
        safeprintf("Device is ESP32\r\n");
    }
    if (esp_device_is_esp8266()) {
        safeprintf("Device is ESP8266\r\n");
    }
    /* Start thread to toggle device present */
    //esp_sys_thread_create(NULL, "device_present", (esp_sys_thread_fn)esp_device_present_toggle, NULL, 0, ESP_SYS_THREAD_PRIO);
    esp_hostname_set("abc", NULL, NULL, 1);
    esp_hostname_get(hn, sizeof(hn), NULL, NULL, 1);
    safeprintf("Hostname: %s\r\n", hn);

    /*
     * Try to connect to preferred access point
     *
     * Follow function implementation for more info
     * on how to setup preferred access points for fast connection
     */
    //start_access_point_scan_and_connect_procedure();
    //esp_sys_thread_terminate(NULL);
    //connect_to_preferred_access_point(1);
    esp_sta_autojoin(1, NULL, NULL, 1);
    esp_sta_join("Majerle WIFI", "majerle_internet_private", NULL, NULL, NULL, 1);

    esp_ping("majerle.eu", &ping_time, NULL, NULL, 1);
    safeprintf("Ping time: %d\r\n", (int)ping_time);

    /*
     * Check if device has set IP address
     *
     * This should always pass
     */
    //if (esp_sta_has_ip()) {

    //    esp_ip_t ip;
    //    uint8_t is_dhcp;

    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    safeprintf("Connected to WIFI!\r\n");
    //    safeprintf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //    esp_sta_setip(&dev_ip, NULL, NULL, 0, NULL, NULL, 1);
    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    safeprintf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //    esp_dhcp_configure(1, 0, 1, 1, NULL, NULL, 1);
    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    safeprintf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //}

    //esp_sta_setip(&dev_ip, NULL, NULL, NULL, NULL, 1);
    //esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);

    /* Start server on port 80 */
    //http_server_start();
    //esp_sys_thread_create(NULL, "netconn_client", (esp_sys_thread_fn)netconn_client_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "netconn_server", (esp_sys_thread_fn)netconn_server_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "netconn_server_single", (esp_sys_thread_fn)netconn_server_1thread_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client", (esp_sys_thread_fn)mqtt_client_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client_api", (esp_sys_thread_fn)mqtt_client_api_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client_api_cayenne", (esp_sys_thread_fn)mqtt_client_api_cayenne_thread, NULL, 0, ESP_SYS_THREAD_PRIO);

    /*if (esp_cayenne_create(&cayenne, &cayenne_mqtt_client_info, cayenne_evt_func) != espOK) {
        safeprintf("Cannot create new cayenne instance!\r\n");
    } else {

    }*/

    /* Notify user */
    

    while (1) {}

    {
        espr_t res;
        esp_pbuf_p pbuf;
        esp_netconn_p client;

        client = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
        if (client != NULL) {
            while (1) {
                res = esp_netconn_connect(client, "10.57.218.183", 123);
                if (res == espOK) {                     /* Are we successfully connected? */
                    safeprintf("Connected to host\r\n");
                    do {
                        res = esp_netconn_receive(client, &pbuf);
                        safeprintf("GOT FROM BUFFER...delaying...\r\n");
                        //esp_delay(5000);
                        if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
                            safeprintf("Connection closed by remote side...\r\n");
                            break;
                        }
                        if (res == espOK && pbuf != NULL) {
                            int len = esp_pbuf_length(pbuf, 1);
                            safeprintf("Received new data packet of %d bytes: %.*s\r\n",
                                len, len,
                                (const char *)esp_pbuf_get_linear_addr(pbuf, 0, NULL));
                            esp_pbuf_free(pbuf);
                            pbuf = NULL;
                        }
                    } while (1);
                } else {
                    safeprintf("Cannot connect to remote host!\r\n");
                }
            }
        }
        esp_netconn_delete(client);             /* Delete netconn structure */
    }

    /* Terminate thread */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Global ESP event function callback
 * \param[in]       evt: Event information
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_evt(esp_evt_t* evt) {
    switch (evt->type) {
        case ESP_EVT_INIT_FINISH: {
            /* Device is not present on init */
            //esp_device_set_present(0, NULL, NULL, 0);
            break;
        }
        case ESP_EVT_RESET: {
            if (esp_evt_reset_get_result(evt) == espOK) {
                safeprintf("Reset sequence successful!\r\n");
            } else {
                safeprintf("Reset sequence error!\r\n");
            }
            break;
        }
        case ESP_EVT_RESTORE: {
            if (esp_evt_restore_get_result(evt) == espOK) {
                safeprintf("Restore sequence successful!\r\n");
            } else {
                safeprintf("Restore sequence error!\r\n");
            }
            break;
        }
        case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            esp_sw_version_t v_min, v_curr;

            esp_get_min_at_fw_version(&v_min);
            esp_get_current_at_fw_version(&v_curr);

            safeprintf("Current ESP8266 AT version is not supported by library\r\n");
            safeprintf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            safeprintf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case ESP_EVT_WIFI_GOT_IP: {
            safeprintf("Wifi got an IP address.\r\n");
            break;
        }
        case ESP_EVT_WIFI_CONNECTED: {
            safeprintf("Wifi just connected. Read access point information\r\n");
            esp_sta_get_ap_info(&connected_ap_info, NULL, NULL, 0);
            break;
        }
        case ESP_EVT_WIFI_DISCONNECTED: {
            safeprintf("Wifi just disconnected\r\n");
            break;
        }
        case ESP_EVT_STA_INFO_AP: {
            safeprintf("SSID: %s, ch: %d, rssi: %d\r\n",
                esp_evt_sta_info_ap_get_ssid(evt),
                (int)esp_evt_sta_info_ap_get_channel(evt),
                (int)esp_evt_sta_info_ap_get_rssi(evt)
            );
            break;
        }
        case ESP_EVT_WIFI_IP_ACQUIRED: {
            esp_ip_t ip;
            uint8_t is_dhcp;

            safeprintf("WIFI IP ACQUIRED!\r\n");
            if (esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp) == espOK) {
                safeprintf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
            } else {
                safeprintf("Acquired IP is not valid\r\n");
            }
            
            break;
        }
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_EVT_AP_CONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_connected_sta_get_mac(evt);
            safeprintf("New station connected to ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_EVT_AP_DISCONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_disconnected_sta_get_mac(evt);
            safeprintf("Station disconnected from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_EVT_AP_IP_STA: {
            esp_mac_t* mac = esp_evt_ap_ip_sta_get_mac(evt);
            esp_ip_t* ip = esp_evt_ap_ip_sta_get_ip(evt);
            safeprintf("Station received IP address from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X and IP: %d.%d.%d.%d\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5],
                ip->ip[0], ip->ip[1], ip->ip[2], ip->ip[3]);
            break;
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        default: break;
    }
    return espOK;
}

static espr_t
esp_conn_evt(esp_evt_t* evt) {
    static char data[] = "test data string\r\n";
    esp_conn_p conn;

    conn = esp_conn_get_from_evt(evt);

    switch (evt->type) {
        case ESP_EVT_CONN_ACTIVE: {
            safeprintf("Connection active!\r\n");
            safeprintf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            safeprintf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            safeprintf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            safeprintf("Close API call: %d\r\n", (int)esp_conn_close(conn, 0));
            safeprintf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            safeprintf("Close API call: %d\r\n", (int)esp_conn_close(conn, 0));

            /*
            esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0);
            esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0);
            */
            break;
        }
        case ESP_EVT_CONN_SEND: {
            espr_t res = esp_evt_conn_send_get_result(evt);
            if (res == espOK) {
                safeprintf("Connection data sent!\r\n");
            } else {
                safeprintf("Connect data send error!\r\n");
            }
            break;
        }
        case ESP_EVT_CONN_RECV: {
            esp_pbuf_p pbuf = esp_evt_conn_recv_get_buff(evt);
            esp_conn_p conn = esp_evt_conn_recv_get_conn(evt);
            safeprintf("\r\nConnection data received: %d / %d bytes\r\n",
                (int)esp_pbuf_length(pbuf, 1),
                (int)esp_conn_get_total_recved_count(conn)
            );
            esp_conn_recved(conn, pbuf);
            break;
        }
        case ESP_EVT_CONN_CLOSE: {
            safeprintf("Connection closed!\r\n");
            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "majerle.eu", 80, NULL, esp_conn_evt, 0);
            break;
        }
        case ESP_EVT_CONN_ERROR: {
            safeprintf("Connection error!\r\n");
            break;
        }
        default: break;
    }
    return espOK;
}