/**
 * \page            page_requirements General requirements
 * \tableofcontents
 *
 * \section         sect_requirement_latest_at_software Latest ESP software.
 *
 * Development of this library follows latest ESP8266 AT software development from Espressif systems.
 *
 *  - ESP8266 must include latest AT software
 *      - Currently supported AT version is <b>1.6.0</b> based on SDK version <b>2.2.0</b> which is not yet released officially from EspressIf Systems
 *      - Precompiled development AT version from Espressif is available on Github of this library
 *      - If you are not sure which version is running on ESP, you may test it using `AT+GMR\r\n` command.
 *
 * \note            In case you do not fulfill these part, please update software on ESP to latest. Check \ref page_update_at_software section.
 */