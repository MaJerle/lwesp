/**
 * \page            page_appnote Application note
 * \tableofcontents
 *
 * This article shows how ESP AT lib works to get understanding what is happening under the hood.
 *
 * \note            Currently, only OS mode is allowed.
 *
 * \section         sect_porting_guide Porting guide
 *
 * \subsection      subsect System structure
 *
 * \image html system_structure.png System structure organization
 *
 * We can describe library structure in 4 different layers:
 *
 *  - <b>User application</b>: User application is highest layer where entire code is implemented by user
 *      and where ESP AT library API functions are called from
 *
 *  - <b>ESP AT middleware</b>: ESP AT middleware layer consists of API functions,
 *      thread management functions and all utilities necessary for smooth operation.
 *
 *  - <b>System functions</b>: Layer where system dependant functions must be implemented,
 *      such as current time in milliseconds and all OS dependant functions for:
 *      - Managing threads
 *      - Managing semaphores
 *      - Managing mutexes
 *      - Managing message queues
 *
 *      More about this part can be found in \ref ESP_SYS section.
 *
 *  - <b>AT port communication functions</b> or <b>ESP LL</b>: Part of code where user must take care
 *      of sending and receiving data from/to ESP AT lib to properly handle communication between
 *      host device and ESP device.
 *      - User must assign memory for memory manager in this section.
            Check \ref ESP_MEM and \ref ESP_LL sections for more information.
 *
 *      More about this part can be found in \ref ESP_LL section.
 *
 *  - <b>ESP physical device</b>: Actual ESP8266 or ESP32 device
 *
 * \subsection      subsect_port_implementation Implementation specific part
 *
 * Before usage, user must implement all functions in \ref ESP_LL section
 * as well as take care of proper communication with ESP device in \ref ESP_LL section.
 *
 * \note            For more information about how to port, check sections accordingly
 * 
 * \section         sect_thread_comm Inter-thread communication
 *
 * In order to have very effective library from resources point of view,
 * an inter-thread communication was introduced.
 *
 * \image html thread_communication.png Inter-Thread communication between user and library.
 *
 * Library consists of 2 threads working in parallel and bunch of different user threads.
 *
 * \subsection      subsec_thread_user User thread(s)
 *
 * User thread is a place where user communicates with ESP AT library.
 * When a new command wants to be executed to ESP device, users calls appropriate API function which will do following steps:
 *
 *  - Allocate memory for command message from memory manager
 *  - Assign command type to message
 *  - Set other parameters, related or required to command
 *  - If user wants to wait for response (blocking mode), then create system semaphore <b>sem</b> and lock it immediatelly
 *  - Send everything to producing message queue which is later read in producing thread
 *  - If user don't want blocking mode, return from function with status OK
 *      otherwise wait for semaphore <b>sem</b> to be released from producing thread
 *      - When <b>sem</b> semaphore is locked, user thread may sleep and release resources for other threads this time
 *  - If user selects blocking mode, wait for response, free command memory in memory manager and return command response status
 *
 * User may use different threads to communicate with ESP AT lib at the same time since memory manager
 * is natively protected by mutex and producing queue is protected from multiple accesses by OS natively.
 *
 * \subsection      subsec_thread_producer Producer thread
 *
 * Producer threads reads message queue with user commands and sends initial AT command to AT port.
 * When there is no commands from user, thread can sleep waiting for new command from user.
 *
 * Once there is a command read from message queue, these steps are performed:
 *
 *  - Check if processing function is set and if command is valid
 *  - Locks <b>sync_sem</b> semaphore for synchronization between processing and producing threads
 *  - Sends initial AT command to AT port according to command type
 *  - Waits for <b>sync_sem</b> to be ready again (released in processing thread)
 *  - If command was blocking, set result and unlock command <b>sem</b> semaphore
 *  - If command was not blocking, free command memory from memory manager
 *
 * \subsection      subsec_thread_process Process thread
 *
 * Processing thread reads received data from AT port and processes them.
 *
 * If command is active and received data belongs to command, they are processed according to command.
 * If received data are not related to command (such as received network data <b>+IPD</b>),
 * they are also processed and callback function is immediatelly called to notify user about received data.
 *
 * Here is a list of some events, which may be read from ESP device without triggering any command:
 *  
 *  - Received network data <b>+IPD</b>
 *  - Station just disconnected from access point <b>WIFI DISCONNECT</b>
 *  - Station just connected to access point <b>WIFI CONNECTED</b>
 *  - ...
 *
 * All these commands must be reported to user. To do this, callback is triggered to notify user.
 *
 * \section         sect_block_nonblock Blocking and non-blocking commands
 *
 * Every command (except if noted otherwise) can be executed in blocking or non-blocking mode.
 *
 * \subsection      subsect_blocking Blocking mode
 *
 * When blocking mode is selected, function will block execution until response is received 
 * and user has immediate result so linear code execution may be applied:
 *
 * \code{c}
//Execute command 1 in blocking mode
if (cmd1(param1, param2, 1) == espOK) {         // Execute cmd 1 in blocking mode and read response immediatelly
    if (cmd2(param3, param4, 1) == espOK) {     // Execute cmd 2 in blocking mode and read response immediatelly
        // Do some job after 2 commands were successfully executed
    }
}
\endcode
 *
 * \warning         When user wants to send command from callback function,
 *                  it is mandatory to call it in non-blocking way, otherwise you may enter dead-lock
 *                  and your program will stop in this position forever.
 *
 * \subsection      subsect_nonblocking Non-blocking mode
 *
 * In non-blocking mode, command is created, sent to producing message queue and function returns without waiting for response from device.
 * This mode does not allow linear programming style, because after non-blocking command, callback function is called.
 *
 * \note            As of now, fully implemented callbacks are done for \ref ESP_CONN API only
 *                  because these are used the most and therefore most focus was applied to this section.
 *
 * \note            When user wants to send command from callback function,
 *                  this is the only allowed way to do it. Every command must be called in
 *                  non-blocking way from callback function.
 *
 * Pseudo code example for non-blocking API call:
 *
 * \code{c}
if (cmd1(param1, param2, 0) == espOK) {
    // Cmd 1 was created and sent to message queue for processing
}

// Callback function for command
callback_fn(evt) {
    if (evt == cmd1_evt) {
        // Command was executed
        // Send another, must always be sent non-blocking way!
        cmd2(param2, param3, 0);
    }
}
\endcode
 *
 */