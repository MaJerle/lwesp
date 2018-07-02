/**
 * \page            page_faq Frequently asked questions
 * \tableofcontents
 *
 * \section         sect_faq_min_at_sw What is the minimum ESP8266 AT software version?
 *
 * This library follows latest AT official releases available on official Espressif website.
 *
 *  - `1.6.0` AT command version is mandatory: https://www.espressif.com/en/products/software/esp-at/resource
 * 	- If you are not sure about your version, use `AT+GMR\r\n` command to test for current AT version
 *
 * \note            In case you do not fulfill these part, please update software on ESP to latest. Check \ref page_update_at_software section.
 *
 * \section 		sect_faq_can_rtos Can I use this library with operating system?
 *
 * You may (and you <b>have to</b>) use this library with and only with operating system (or RTOS).
 * Library has advanced techniques to handle `AT` based software approach which is very optimized when used
 * with operating system and can optimize user application program layer.
 */