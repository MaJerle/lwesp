# Changelog

## Develop

- Change license year to 2022
- Improve MQTT module implementation
- Improve ThreadX port
- Enable manual TCP receive by default, to improve system stability
- Add MQTT cayenne async demo, publish-mode only through ring buffer
- Timeout module returns ERRMEM if no memory to allocate block
- Add esp_at_binaries from Espressif, used for library verification (official AT firmware)
- Add optional `AT+CMD?` command at reset/restore process, for debug purpose for the moment, only
- Add function to get ESP device used for AT command communication
- Fix `lwesp_get_min_at_fw_version` to return min AT version for detected ESP device
- Improve SNTP module comments, change timezone variable to `int16_t`
- Implement global SNTP callback when command is to obtain current time
- Add option to get response to `ERR CODE:` message if command doesn't exist and put it to result of command execution

## 1.1.2-dev

- Add POSIX-compliant low-level driver (thanks to community to implement it)
- Prohibit transmission of too long UDP packets (default), can be disabled with configuration option
- Split CMakeLists.txt files between library and executable
- Move `esp_set_server` function to separate file `lwesp_server.c`
- Use `AT+GMR` command just after reset/restore to determine ESP device being connected on AT port
- Minimum required AT binaries are now `2.3.0` for `ESP32/ESP32C3` and `2.2.1` for `ESP8266`
- Connection status is acquired with `AT+CIPSTATE` or `AT+CIPSTATUS`, depends on Espressif connected device
- Add optional full fields for access point scan with `LWESP_CFG_ACCESS_POINT_STRUCT_FULL_FIELDS` config option
- Add optional keep-alive periodic timeout to system event callback functions. Can be used to act as generic timeout event
- Improve station manager snippet with asynchronous mode

## v1.1.1-dev

- Update to support library.json for Platform.IO

## v1.1.0-dev

- Add support for SDK v2.2
- Extend number of information received on AP scan
  - Add option for `WPA3` and `WPA2_WPA3_PSK` authentication modes
  - Add bgn and wps readings
- Add support for IPv6
- Add option to disconnect all stations from Soft-AP
- TODO: Add DNS for IPv6 support (Optional)
- TODO: Add support for WIFI GOT IP to parse IPv6
- Update CMSIS OS driver to support FreeRTOS aware kernel

## v1.0.0

- First stable release
- Works with *esp-at* version `v2.1.0`
- Implements all basic functionality for ESP8266 and ESP32
- Added operating system-based sequential API
- Other bug fixes and docs updates

## v0.6.1

- Fixed inadequate MQTT RX data handling causing possible overflow of memory
- Added support for zero-copy MQTT RX data

## v0.6.0

- Added support for ESP32 & ESP8266
- Official support for ESP32 AT firmware v1.2 & ESP8266 AT firmware v2.0
- Added examples to main repository
- Preparation for BLE support in ESP32
- Removed AT commands with `_CUR` and `_DEF` suffixes
- Renamed some event names, such as `ESP_EVT_CONN_CLOSE` instead of `ESP_EVT_CONN_CLOSED`
- Added DHCP/static IP support
- Added CMSIS-OS v2 support
- Added LwMEM port for dynamic memory allocation
- Other bug fixes

## v0.5.0

- Remove `_t` for every struct/enum name
- Fully use ESP_MEMCPY instead of memcpy
- When connection is in closing mode, stop sending any new data and return with error
- Remove `_data` part from event helper function for connection receive
- Implement semaphores in internal threads
- Add driver for NUCLEO-F429
- Implement timeout callback for major events when device does not reply in given time
- Add callback function support to every API function which directly interacts with device
- Replace all files to CRLF ending
- Replace `ESP_EVT_RESET` to `ESP_EVT_RESET_DETECTED`
- Replace `ESP_EVT_RESET_FINISH` to ESP_EVT_RESET`
- Replace all header files guards with ESP_HDR_ prefix
- Add espERRBLOCKING return when function is called in blocking mode when not allowed
- Other bug fixes to stabilize AT communication

## v0.4.0

- Add sizeof for every memory allocation
- Function typedefs suffix has been renamed to `_fn` instead of `_t`
- Merge events for connection data send and data send error
- Send callback if sending data is not successful in any case (timeout, ERROR, etc)
- Add functions for IP/port retrieval on connections
- Remove goto statements and use deep if statements
- Fix MQTT problems with username and password
- Make consistent variable types across library

## v1.3.0

- Rename all cb annotations with evt, replacing callbacks with events,
- Replace built-in memcpy and memset functions with `ESP_MEMCPY` and `ESP_MEMSET` to allow users to do custom implementation
- Added example for Server RTOS
- Added API to unchain first pbuf in pbuf chain
- Implemented first prototype for manual TCP receive functionality.

## v0.2.0

- Fixed netconn issue with wrong data type for OS semaphore
- Added support for asynchronous reset
- Added support for tickless sleep for modern RTOS systems

## v0.1.0

- Initial release
