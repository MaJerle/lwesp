/**
 * \addtogroup      ESP_CONN
 * \{
 *
 * Connection based functions to manage sending and receiving data
 *
 * \note            Functions are in general thread safe. If there is an expection, it is mentioned in function description
 *
 * In the below example, you can find frequent use case how to use connection API in non-blocking callback mode.
 *
 * \par             Example
 *
 * In this example, most useful non-blocking approach is used to handle the connection.
 *
 * \code{c}
/* Request data are sent to server once we are connected */
uint8_t req_data[] = ""
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Connection: close\r\n"
    "\r\n";

/*
 * \brief           Connection callback function
 *                  Called on several connection events, such as connected, closed, data received, data sent, ...
 * \param[in]       cb: ESP callback event
 */ 
static espr_t
conn_cb(esp_cb_t* cb) {
    esp_conn_p conn = esp_conn_get_from_evt(cb);/* Get connection from current event */
    if (conn == NULL) {
        return espERR;                          /* Return error at this point as this should never happen! */ 
    }
    
    switch (cb->type) {
        /*
         * A new connection just became active
         */
        case ESP_CB_CONN_ACTIVE: {
            printf("Connection active!\r\n");
            
            /*
             * After we are connected,
             * send the HTTP request string in non-blocking way
             */
            esp_conn_send(conn, req_data, sizeof(req_data) - 1, NULL, 0);
            break;
        }
        
        /*
         * Connection closed event
         */
        case ESP_CB_CONN_CLOSED: {
            printf("Connection closed!\r\n");
            if (cb->cb.conn_active_closed.forced) { /* Was it forced by user? */
                printf("Connection closed by user\r\n");
            } else {
                printf("Connection closed by remote host\r\n");
            }
        }
        
        /*
         * Data received on connection
         */
        case ESP_CB_CONN_DATA_RECV: {
            esp_pbuf_p pbuf = cb->cb.conn_data_recv.buff;   /* Get data buffer */
            
            /*
             * Connection data buffer is automatically
             * freed when you return from function
             * If you still want to hold it,
             * then either chain it using esp_pbuf_chain
             * or reference it using esp_pbuf_ref functions.
             * Remember!!
             * After you do this, you are responsible for
             * freeing the memory otherwise memory leak will appear in system!
             * Use esp_pbuf_free once you don't need data anymore
             */
            
            printf("Connection data received!\r\n");
            if (pbuf != NULL) {
                size_t len;
                /*
                 * You should not call esp_pbuf_free on this variable unless
                 * you used esp_pbuf_ref before to increase reference
                 */
                len = esp_pbuf_length(pbuf, 1); /* Get total length of buffer */
                printf("Length of data: %d bytes\r\n", (int)len);
            }
        }
        default:
            break;
    }
    return espOK;
}

/*
 * \brief           Thread function
 */
static void
thread_or_main_func(void) {
    /*
     * Start the connection in non-blocking way and set the
     * function argument to NULL and callback function to conn_cb
     */
    esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "example.com", 80, NULL, conn_cb, 0);
    
    // Do other tasks...
}

\endcode
 *
 * \}
 */
/**
 * \addtogroup      ESP_CONN
 * \{
 *
 * \section         sect_send_data Send data methods
 *
 * User can choose between <b>2</b> different methods for sending the data:
 *
 *  - Temporary connection write buffer
 *  - Send every packet separately
 *
 * \par             Temporary connection write buffer
 *
 * When user calls \ref esp_conn_write function,
 * a temporary buffer is created on connection and data are copied to it,
 * but they might not be send to command queue for sending.
 *
 * ESP can send up to <b>x</b> bytes at a time in single AT command,
 * currently limited to <b>2048</b> bytes.
 * If we can optimize packets of <b>2048</b> bytes,
 * we would have best throughput speed and this is the purpose of write function.
 *
 * \note            If we write bigger amount than max data for packet
 *                  function will automaticaly split data to make sure,
 *                  everything is sent in correct order
 *
 * \code
size_t rem_len;
esp_conn_p conn;
espr_t res;

/* ... other tasks to make sure connection is established */

/* We are connected to server at this point! */
/*
 * Call write function to write data to memory
 * and do not send immediately unless buffer is full after this write
 *
 * rem_len will give us response how much bytes
 * is available in memory after write
 */
res = esp_conn_write(conn, "My string", 9, 0, &rem_len);
if (rem_len == 0) {
    printf("No more memory available for next write!\r\n");
}
res = esp_conn_write(conn, "Example.com", 11, 0, &rem_len);

/*
 * Data will stay in buffer until buffer is full,
 * except if user wants to force send,
 * call write function with flush mode enabled
 *
 * It will send out together 20 bytes
 */
esp_conn_write(conn, NULL, 0, 1, NULL);

\endcode
 *
 * \}
 */
/**
 * \addtogroup      ESP_CONN
 * \{
 *
 * \par             Send every packet separately
 *
 * If you are not able to use write buffer,
 * due to memory constraints, you may also send data
 * by putting every write command directly to command message queue.
 *
 * Use \ref esp_conn_send or \ref esp_conn_sendto functions,
 * to send data directly to message queue.
 *
 * \}
 */