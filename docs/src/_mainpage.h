/**
 * \mainpage
 * \tableofcontents
 * 
 * ESP AT is generic and advanced library for communicating to ESP8266 and ESP32 
 * WiFi transceivers using AT commands from host MCU.
 *
 * It is intented to work with embedded systems and is device and platform independent.
 *
 * \section         sect_features Features
 *
 *  - Supports latest ESP8266 and ESP32 AT commands software
 *  - Platform independent and very easy to port
 *  - Written in C language (C99)
 *  - Allows different configurations to optimize user requirements
 *  - Supports implementation with operating systems with advanced inter-thread communications
 *      - Currently only OS mode is supported
 *      - 2 different threads handling user data and received data
 *          - First (producer) thread (collects user commands from user threads and starts the command processing)
 *          - Second (process) thread reads the data from ESP device and does the job accordingly
 *  - Includes several applications built on top of library:
 *      - HTTP server with dynamic files (file system) support
 *      - MQTT client (under development)
 *
 *
 * \section         sect_resources Resources
 *
 *  - <a href="https://github.com/MaJerle/ESP_AT_Lib">Official development repository on Github</a>
 *  - <a href="http://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf">Official AT commands instruction set by Espressif systems</a>
 *
 *
 * \section         sect_license License
 *
 */