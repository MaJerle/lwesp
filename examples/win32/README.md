# WIN32 Examples

All windows based examples are written in Visual Studio 2017 as "Win32 project" and "Console Application".

## NodeMCU development board

For development purposes, NodeMCU v3 board was used with virtual COM port support
to translate USB communication to UART required for ESP8266.

> Some NodeMCU boards have [CH340 USB->UART](https://www.tindie.com/products/multicognitive/nodemcu-esp8266-v3-lua-ch340-wifi-dev-board/) transceiver where I found problems with communication due to data loss between ESP and PC even at 115200 bauds. Try to find [NodeMCU with something else than CH340](https://www.ebay.com/itm/NodeMcu-Amica-V3-ESP-12E-ESP12E-4MB-FLASH-Lua-WIFI-Networking-dev-board-ESP8266-/141778019163).
