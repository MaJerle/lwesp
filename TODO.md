# TODO list

- Implement `AT+CWSTATE` to query Wi-Fi state
- Implement `AT+CIPSENDL` and `AT+CIPSENDLCFG` for long data send
- Implement `AT+CIPTCPOPT` to configure TCP connection
- Implement BLE feature
- Implement BT feature
- Transfer Cayenne MQTT app to be based on non-blocking API instead
- Add DNS for IPv6 support (Optional)
- Add `AT+CWJEAP` for WPA2 connections
- Implement single callback when station is connected and IP is received (so far STA_GOT_IP may be called several times in a row, when IP v4 and v6 are received)
   - Implement new type of event instead, that is called only once per connection
- Add support for wifi mode `0` that has disabled WIFI IP