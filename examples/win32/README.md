# WIN32 Examples

All windows based examples are written in Visual Studio 2017 as "Win32 project" and "Console Application".

## NodeMCU development board

For development purposes, NodeMCU v3 board was used with virtual COM port support
to translate USB communication to UART required for ESP8266.

> Some NodeMCU boards have [CH340 USB->UART](https://www.tindie.com/products/multicognitive/nodemcu-esp8266-v3-lua-ch340-wifi-dev-board/) transceiver where I found problems with communication due to data loss between ESP and PC even at 115200 bauds. Try to find [NodeMCU with something else than CH340](https://www.ebay.com/itm/NodeMcu-Amica-V3-ESP-12E-ESP12E-4MB-FLASH-Lua-WIFI-Networking-dev-board-ESP8266-/141778019163).

## System functions for WIN32

Required system functions are based on "windows.h" file, available on windows operating system. Natively, there is support for:
1. Timing functions
2. Semaphores
3. Mutexes
4. Threads

The last part are message queues which are not implemented in Windows OS. Message queues were developed with help of semaphores and dynamic memory allocatations. System port for WIN32 is available in [esp_sys_win32.c](/src/system/esp_sys_win32.c) file.

## Communication with WIN32

Communication with NodeMCU hardware is using virtual files for COM ports. 
Implementation of low-level part (together with memory allocation for library) is available in [esp_ll_win32.c](/src/system/esp_ll_win32.c) file.

> In order to start using this port, user must set the appropriate COM port name when opening a virtual file. Please check implementation file for details.

### Visual Studio configuration

It may happen that Visual Studio sets different configuration on first project load and this may lead to wrong build and possible errors. Active configuration must be `Debug` and `Win32` or `x86`. Default active build can be set in project settings.
