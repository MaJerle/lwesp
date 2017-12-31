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
 * \include         _example_ll.c
 *
 * \}
 */