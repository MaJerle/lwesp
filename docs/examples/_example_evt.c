/*
 * \brief           User defined callback function for ESP events
 * \param[in]       evt: Callback event data
 */
espr_t
esp_callback_function(esp_evt_t* evt) {
    switch (evt->type) {                         /* Check event type */
        case ESP_CB_RESET: {                     /* Reset detected on ESP device */
            /* Option 1 */
            if (esp_evt_reset_is_forced(evt)) {  /* Check if forced by user */
                printf("Reset forced by user!\r\n");
            }
            /* Option 2 */
            if (esp->evt.evt.reset.forced) {     /* Check if forced by user */
                printf("Reset forced by user!\r\n");
            }
            break;
        }
        default: break;
    }
    return espOK;
}