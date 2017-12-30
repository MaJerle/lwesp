/**
 * \addtogroup      ESP_LL
 * \{
 *
 * Low-level communication part is responsible to make sure
 * all bytes received from ESP device are properly 
 * sent to upper layer stack and that all bytes are sent to ESP
 * when requested by upper layer ESP stack
 *
 * When initializing low-level part, following steps are important and must be done when \ref esp_ll_init function is called:
 *
 *  1. Assign memory for dynamic allocations required by ESP library
 *  2. Configure AT send function to use when we have data to be transmitted
 *  3. Configure AT port to be able to send/receive any data
 *
 * \par             Example
 * 
 * Example shows basic functionality what user MUST do in order to prepare stack properly.
 *
 * \code{c}
uint8_t initialized = 0;

/*
 * \brief           Send function callback which is called each time user must send something to AT port
 * \param[in]       data: Data to send
 * \param[in]       len: Number of bytes to send
 * \return          Number of bytes sent
 */
static uint16_t
send_data(const void* data, uint16_t len) {
    return len;
}

/*
 * \brief           Core callback function which must be implemented by user
 * \param[in]       ll: Low-Level structure
 * \param[in]       baudrate: Baudrate for AT port
 * \return          espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_ll_init(esp_ll_t* ll, uint32_t baudrate) {
    /*
     * In step 1, define memory array used for memory allocator
     * and send it to upper layer.
     *
     * Define memory regions where allocater may 
     * search for available memory.
     *
     * Since function may be called multiple times,
     * make sure you assign memory only first time function is called
     */
    static uint8_t memory[0x10000];
    esp_mem_region_t mem_regions[] = {
        { memory, sizeof(memory) }
    };
    if (!initialized) {
        esp_mem_assignmemory(mem_regions, ESP_ARRAYSIZE(mem_regions)); 
    }
    
    /*
     * Step 2 is to set the send callback function
     * which is called each time data have to be sent to AT port
     */
    if (!initialized) {
        ll->send_fn = send_data;
    }
    
    /*
     * In last step we have to configure AT port
     * to be able to receive and transmit data
     *
     * Since user may change baudrate in upper layer,
     * this function may be called multiple times
     */
    configure_uart(baudrate);
    
    initialized = 1;
}
\endcode
 *
 * \}
 */