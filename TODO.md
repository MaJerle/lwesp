# TODO list

- Implement `AT+CWSTATE` to query Wi-Fi state
- Implement `AT+CIPSENDL` and `AT+CIPSENDLCFG` for long data send
- Implement `AT+CIPTCPOPT` to configure TCP connection
- Implement BLE feature
- Add DNS for IPv6 support (Optional)
- Add support for WIFI GOT IP to parse IPv6
- Add `AT+CWJEAP` for WPA2 connections
- Implement single callback when station is connected and IP is received (so far STA_GOT_IP may be called several times in a row, when IP v4 and v6 are received)
   - Implement new type of event instead, that is called only once per connection
- Check supported commands to determine `AT+CIPSTATUS` or `AT+CIPSTATE` command to obtain connection status