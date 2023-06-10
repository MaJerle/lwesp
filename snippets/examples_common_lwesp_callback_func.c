#include <stdio.h>
#include "examples_common_lwesp_callback_func.h"
#include "lwesp/lwesp.h"

/**
 * \brief           Core LwESP callback function for all examples in the repository.
 * 
 * This reduces redundancy of the same code being written multiple times.
 * 
 * \param[in]       evt: Event data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
examples_common_lwesp_callback_func(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
        case LWESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            lwesp_sw_version_t v_min, v_curr;

            lwesp_get_min_at_fw_version(&v_min);
            lwesp_get_current_at_fw_version(&v_curr);

            printf("Current ESP[8266/32[-C3]] AT version is not supported by the library\r\n");
            printf("Minimum required AT version is: %08X\r\n", (unsigned)v_min.version);
            printf("Current AT version is: %08X\r\n", (unsigned)v_curr.version);
            break;
        }
        case LWESP_EVT_INIT_FINISH: {
            printf("Library initialized!\r\n");
            break;
        }
        case LWESP_EVT_RESET_DETECTED: {
            printf("Device reset detected!\r\n");
            break;
        }
        default: break;
    }
    return lwespOK;
}
