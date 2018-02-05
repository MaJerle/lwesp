/**
 * \page            page_requirements General requirements
 * \tableofcontents
 *
 * \section         sect_requirement_latest_at_software Latest ESP software.
 *
 * Development of this library follows latest ESP8266 AT software development by Espressif systems.
 *
 *  - ESP8266 must include latest AT software
 *      - Currently supported AT version is `1.6.0` based on SDK version `2.2.0` which is not yet officially released by EspressIf Systems
 *      - Precompiled development AT version from Espressif is available on Github of this library: https://github.com/MaJerle/ESP_AT_Lib/tree/master/bin
 *      - If you are not sure which version is running on ESP, you may test it using `AT+GMR\r\n` command.
 *
 * \note            In case you do not fulfill these part, please update software on ESP to latest. Check \ref page_update_at_software section.
 */