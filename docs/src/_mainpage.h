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
 *      - \ref ESP_APP_HTTP_SERVER with dynamic files (file system) support
 *      - \ref ESP_APP_MQTT_CLIENT for MQTT connection
 *
 *
 * \section         sect_requirements Requirements
 *
 * In order to start with this library, you must complete these requirements:
 *
 *  - ESP8266 must include latest AT software
 *      - Currently supported is AT 1.6.0 based on SDK 2.2.0 which is not yet released officially from EspressIf Systems
 *      - Precompiled development AT version from Espressif is available on Github account of this library
 *
 * \section         sect_resources Resources
 *
 *  - Read \ref page_appnote before you start development
 *  - <a href="https://github.com/MaJerle/ESP_AT_Lib">Official development repository on Github</a>
 *  - <a href="http://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf">Official AT commands instruction set by Espressif systems</a>
 *
 * \section         sect_license License
 *
 * \verbatim        
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE. \endverbatim
 *
 */