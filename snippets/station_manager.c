/*
 * Station manager to connect station to access point.
 *
 * It is consider as a utility module, simple set of helper functions
 * to quickly connect to access point.
 * 
 * It utilizes 2 different modes, sequential or asynchronous.
 * 
 * Sequential:
 * ==========
 * Call connect_to_preferred_access_point function to connect to access point
 * in blocking mode until being ready to move forward.
 * 
 * Asynchronous:
 * ============
 * Call station_manager_connect_to_access_point_async_init to initialize
 * asynchronous connect mode and activity will react upon received LwESP events to application.
 * 
 * Define list of access points:
 * ============================
 * Have a look at "ap_list_preferred" variable and define
 * list of preferred access point's SSID and password.
 * Ordered by "most preferred" at the lower array index.
 */
#include "station_manager.h"
#include "utils.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Private access-point and station management system
 * 
 * This is used for asynchronous connection to access point
 */
typedef struct {
    size_t index_preferred_list;                /*!< Current index position of preferred array */
    size_t index_scanned_list;                  /*!< Current index position in array of scanned APs */

    uint8_t command_is_running;                 /*!< Indicating if command is currently in progress */
} prv_ap_data_t;

/* Arguments for callback function */
#define ARG_SCAN                (void*)1
#define ARG_CONNECT             (void*)2

/* Function declaration */
static void prv_cmd_event_fn(lwespr_t status, void* arg);
static void prv_try_next_access_point(void);

/*
 * List of preferred access points for ESP device
 * SSID and password
 *
 * ESP will try to scan for access points
 * and then compare them with the one on the list below
 */
static const ap_entry_t ap_list_preferred[] = {
    //{ .ssid = "SSID name", .pass = "SSID password" },
    { .ssid = "TilenM_ST", .pass = "its private" },
    { .ssid = "Kaja", .pass = "ginkaja2021" },
    { .ssid = "Majerle WIFI", .pass = "majerle_internet_private" },
    { .ssid = "Majerle AMIS", .pass = "majerle_internet_private" },
};
static lwesp_ap_t ap_list_scanned[100];         /* Scanned access points information */
static size_t ap_list_scanned_len = 0;          /* Number of scanned access points */
static prv_ap_data_t ap_async_data;             /* Asynchronous data structure */

/* Command to execute to start scanning access points */
#define prv_scan_ap_command_ex(blocking)        lwesp_sta_list_ap(NULL, ap_list_scanned, LWESP_ARRAYSIZE(ap_list_scanned), &ap_list_scanned_len, NULL, NULL, (blocking))
#define prv_scan_ap_command()                   do {\
    if (!ap_async_data.command_is_running) {    \
        ap_async_data.command_is_running = lwesp_sta_list_ap(NULL, ap_list_scanned, LWESP_ARRAYSIZE(ap_list_scanned), &ap_list_scanned_len, prv_cmd_event_fn, ARG_SCAN, 0) == lwespOK;   \
    }       \
} while (0)

/**
 * \brief           Every internal command execution callback
 * \param[in]       status: Execution status result
 * \param[in]       arg: Custom user argument
 */
static void
prv_cmd_event_fn(lwespr_t status, void* arg) {
    LWESP_UNUSED(status);
    /*
     * Command has now successfully finish
     * and callbacks have been properly processed
     */
    ap_async_data.command_is_running = 0;

    if (arg == ARG_SCAN) {
        /* Immediately try to connect to access point after successful scan*/
        prv_try_next_access_point();
    }
}

/**
 * \brief           Try to connect to next access point on a list 
 */
static void
prv_try_next_access_point(void) {
    uint8_t tried = 0;

    /* No action to be done if command is currently in progress or already connected to network */
    if (ap_async_data.command_is_running
        || lwesp_sta_has_ip()) {
        return;
    }

    /*
     * Process complete list and try to find suitable match
     *
     * Use global variable for indexes to be able to call function multiple times
     * and continue where it finished previously
     */

    /* List all preferred access points */
    for (; ap_async_data.index_preferred_list < LWESP_ARRAYSIZE(ap_list_preferred);
            ap_async_data.index_preferred_list++, ap_async_data.index_scanned_list = 0) {

        /* List all scanned access points */
        for (; ap_async_data.index_scanned_list < ap_list_scanned_len; ap_async_data.index_scanned_list++) {

            /* Find a match if available */
            if (strncmp(ap_list_scanned[ap_async_data.index_scanned_list].ssid,
                        ap_list_preferred[ap_async_data.index_preferred_list].ssid,
                        strlen(ap_list_preferred[ap_async_data.index_preferred_list].ssid)) == 0) {

                /* Try to connect to the network */
                if (!ap_async_data.command_is_running
                    && lwesp_sta_join(ap_list_preferred[ap_async_data.index_preferred_list].ssid,
                                    ap_list_preferred[ap_async_data.index_preferred_list].pass,
                                    NULL, prv_cmd_event_fn, ARG_CONNECT, 0) == lwespOK) {
                    ap_async_data.command_is_running = 1;

                    /* Go to next index for sub-for loop and exit */
                    ap_async_data.index_scanned_list++;
                    tried = 1;
                    goto stp;
                } else {
                    /* We have a problem, needs to resume action in next run */
                }
            }
        }
    }

    /* Restart scan operation if there was no try to connect and station has no IP */
    if (!tried && !lwesp_sta_has_ip()) {
        prv_scan_ap_command();
    }
stp:
    return;
}

/**
 * \brief           Private event function for asynchronous scanning
 * \param[in]       evt: Event information
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
prv_evt_fn(lwesp_evt_t* evt) {
    switch (evt->type) {
        case LWESP_EVT_KEEP_ALIVE:
        case LWESP_EVT_WIFI_DISCONNECTED: {
            /* Try to connect to next access point */
            prv_try_next_access_point();
            break;
        }
        case LWESP_EVT_STA_LIST_AP: {
            /*
             * After scanning gets completed
             * manually reset all indexes for comparison purposes
             */
            ap_async_data.index_scanned_list = 0;
            ap_async_data.index_preferred_list = 0;

            /* Actual connection try is done in function callback */
            break;
        }
        default: break;
    }
    return lwespOK;
}

/**
 * \brief           Initialize asynchronous mode to connect to preferred access point
 *
 * Asynchronous mode relies on system events received by the application,
 * to determine current device status if station is being, or not, connected to access point.
 *
 * When used, async acts only upon station connection change through callbacks,
 * therefore it does not require additional system thread or user code,
 * to be able to properly handle preferred access points.
 * This certainly decreases memory consumption of the complete system.
 *
 * \ref LWESP_CFG_KEEP_ALIVE feature must be enable to properly handle all events
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
station_manager_connect_to_access_point_async_init(void) {
    /* Register system event function */
    lwesp_evt_register(prv_evt_fn);

    /*
     * Start scanning process in non-blocking mode
     *
     * This is the only command being executed from non-callback mode,
     * therefore it must be protected against other threads trying to access the same core
     */
    lwesp_core_lock();
    prv_scan_ap_command();
    lwesp_core_unlock();

    /* Return all good, things will progress (from now-on) asynchronously */
    return lwespOK;
}

/**
 * \brief           Connect to preferred access point in blocking mode
 * 
 * This functionality can only be used if non-blocking approach is not used
 * 
 * \note            List of access points should be set by user in \ref ap_list structure
 * \param[in]       unlimited: When set to 1, function will block until SSID is found and connected
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
lwespr_t
connect_to_preferred_access_point(uint8_t unlimited) {
    lwespr_t eres;
    uint8_t tried;

    /*
     * Scan for network access points
     * In case we have access point,
     * try to connect to known AP
     */
    do {
        if (lwesp_sta_has_ip()) {
            return lwespOK;
        }

        /* Scan for access points visible to ESP device */
        printf("Scanning access points...\r\n");
        if ((eres = prv_scan_ap_command_ex(1)) == lwespOK) {
            tried = 0;

            /* Print all access points found by ESP */
            for (size_t i = 0; i < ap_list_scanned_len; i++) {
                printf("AP found: %s, CH: %d, RSSI: %d\r\n", ap_list_scanned[i].ssid, ap_list_scanned[i].ch, ap_list_scanned[i].rssi);
            }

            /* Process array of preferred access points with array of found points */
            for (size_t j = 0; j < LWESP_ARRAYSIZE(ap_list_preferred); j++) {

                /* Go through all scanned list */
                for (size_t i = 0; i < ap_list_scanned_len; i++) {

                    /* Try to find a match between preferred and scanned */
                    if (strncmp(ap_list_scanned[i].ssid, ap_list_preferred[j].ssid, strlen(ap_list_scanned[i].ssid)) == 0) {
                        tried = 1;
                        printf("Connecting to \"%s\" network...\r\n", ap_list_preferred[j].ssid);

                        /* Try to join to access point */
                        if ((eres = lwesp_sta_join(ap_list_preferred[j].ssid, ap_list_preferred[j].pass, NULL, NULL, NULL, 1)) == lwespOK) {
                            lwesp_ip_t ip;
                            uint8_t is_dhcp;

                            printf("Connected to %s network!\r\n", ap_list_preferred[j].ssid);

                            lwesp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
                            utils_print_ip("Station IP address: ", &ip, "\r\n");
                            printf("; Is DHCP: %d\r\n", (int)is_dhcp);
                            return lwespOK;
                        } else {
                            printf("Connection error: %d\r\n", (int)eres);
                        }
                    }
                }
            }
            if (!tried) {
                printf("No access points available with preferred SSID!\r\nPlease check station_manager.c file and edit preferred SSID access points!\r\n");
            }
        } else if (eres == lwespERRNODEVICE) {
            printf("Device is not present!\r\n");
            break;
        } else {
            printf("Error on WIFI scan procedure!\r\n");
        }
        if (!unlimited) {
            break;
        }
    } while (1);
    return lwespERR;
}
