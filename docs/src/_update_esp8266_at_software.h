/**
 * \page            page_update_at_software Update ESP8266 AT software
 * \tableofcontents
 *
 * This section will describe how to update ESP8266 AT commands software to latest version,
 * which is provided from Espressif Systems and is still under development process on their side.
 *
 * \section         sect_update_process_preparations Before you start
 *
 * ESP8266 is a microcontroller itself which supports (like any other MCU) software to be executed.
 * AT commands software is a "project" which implements AT commands for ESP8266 module and is developed by Espressif Systems.
 *
 * When you bought module (or any other source) you probably got AT software provided by Ai-Thinker preloaded.
 * Ai-Thinker is a company which made famous ESP8266 modules ESP-01 to ESP-12 and are all
 * available on Google search for review if needed
 *
 * \note            To test your default AT software running on ESP device, use `AT+GMR` command
 *
 * Espressif Systems has its own SDK (Software development kit) and they also provide AT commands program build on this SDK.
 * 
 * \note            <a href="https://github.com/espressif/ESP8266_NONOS_SDK">Official SDK</a> used to build AT commands software
 * 
 * \section         sect_update_process_hardware Hardware requirements for software
 *
 * Espressif Systems provide 2 AT version releases:
 *
 * - AT release with OTA (at least `8-Mbit` flash memory)
 * - AT release without OTA (at least `4-Mbit` flash memory)
 *
 * \par             AT with OTA (BOOT mode)
 *
 * OTA enables you to run `AT+CIUPDATE` command and module will try to connect to Espressif servers and download newest
 * software for AT commands if it exists. It if does, it will burn it to flash and from there, each new reset will start on 
 * new software.
 *
 * This option requires dual-bank flash with at least `8-Mbit` of flash memory which is included on every ESP8266 module you buy on eBay.
 * If your module does not have at least 8Mbit flash memory, you can't use OTA and `AT+CIUPDATE` will not be enabled in your program.
 *
 * Most of new ESP modules come with 8-Mbit module, however if you have old module, it may still include `4-Mbit` only.
 *
 * \par AT without OTA (Non BOOT mode)
 *
 * AT commands software without OTA requries `4-Mbit` flash memory and basically works on any ESP module, even if bought at very beginning
 * when these modules were first time sold.
 *
 * \section         sect_update_process_download Download AT software
 *
 * You can always get latest NON_OS SDK from Espressif Official website.
 * Latest version when writing this tutorial is NONOS_SDK 2.1 and is available <a href="http://espressif.com/en/support/download/sdks-demos"><b>here</b></a>.
 *
 * \note            Few notes:
 *                      - SDK also includes compiled AT project inside `bin` directory in package.
 *                      - Before you start with update process, always make sure you have latest version downloaded.
 *                      - Espressif developes AT software regulary together with SDK, but official releases are not published for every new feature.
 *                          To be able to follow Espressif developments, you can use precompiled binary files which come together on
 *                          official repository for <a href="https://github.com/MaJerle/ESP_AT_Lib"><b>ESP-AT library</b></a>.
 *                          Upgrade files are available in `bin` directory.
 *
 * \section         sect_update_process_download_flashtool Download flash tool
 *
 * Espressif provides download flash tool, available on website in section
 * <a href="http://espressif.com/en/support/download/other-tools"><b>other tools</b></a>.
 *
 * \section         sect_update_process_hardware_preparation Prepare ESP8266 module
 *
 * Before you can start updating software on ESP8266 module, make sure you have correct wiring.
 * 
 * For wiring, you may use table below, to prepare pins before you can execute upload command
 *
\verbatim
PIN           ESP-01       ESP-07/12    Description

GND           GND          GND          Ground power supply
VCC           VCC          VCC          3.3 (!) V power supply. At least about 200mA should be available from supply for ESP spikes it can generate
TXD           TXD          TXD          This is TXD pin from ESP. It should be connected to RX pin of your USB<->UART converter.
RXD           RXD          RXD          This is RXD pin to ESP. It should be connected to TX pin of your USB<->UART converted.
GPIO0         VCC          VCC          When tied to VCC, ESP8266 is in normal mode, when tied to GND, ESP8266 is in bootloader mode ready to be updated!
GPIO2         VCC          VCC          Boot mode selection
GPIO15        NC           GND          This pin in not available on ESP-01 module and is connected to GND. On ESP-07 must be manually connected to GND
RST           VCC          VCC          Connected to VCC using 10k pull-up resistor to easy reset it when necessary
EN (CH_PD)    VCC          VCC          Connected to VCC to enable ESP8266 operation
\endverbatim
 *
 * \warning         GPIO pin levels should not be greater than 3.6V or you will blow ESP!
 * 
 * \note            <b>Wiring for updating is the same as in normal operation, except GPIO0 must be tied LOW instead of HIGH</b>
 *
 * \par             Boot modes
 *
 * ESP8266 supports 3 boot modes and it uses 3 GPIO pins to achieve this (2 would be enough):
 *
\verbatim
GPIO0   GPIO2   GPIO15     BOOT MODE

LOW     HIGH    LOW        Serial programming mode, ready to update
HIGH    HIGH    LOW        Boot from flash (normal operation for ESP-01 to ESP-12 modules)
ANY     ANY     HIGH       Boot from SDCARD (if connected to SDIO port)
\endverbatim
 *
 * \note            To start update process, connect ESP8266 module with USB<->UART converter and set <b>GPIO0 pins LOW</b> and toggle <b>RST pin from high to low and back</b> to reset module
 *
 * \section         sect_update_process_start Start update process
 *
 * Step-by-step tutorial how to write new software to ESP device
 *
 * \par             Step 1: Prepare software
 *
 * We have downloaded AT software and flash download tool. Now it's time to extract sources.
 * I used <b>D:/TUTORIAL</b> on my Windows as a root folder for updating ESP8266 module.
 * You can use the same or use any other.
 *
 * Extract both files into that folder. 
 *
 * \image html update_process_step_1.png "Folder structure after extracting both files into root directory"
 *
 * \par             Step 2: Configure flash tool
 *
 * In folder "FLASH_DOWNLOAD..." is .exe. Open it and go back to root directory and then to <b>at</b> or <b>upgrade</b> directory (later named <b>source</b> folder).
 *
 * \image html update_process_step_2.png "AT directory with opened flash download tool"
 *
 * \par             Step 3: Set paths and locations for uploading
 *
 * <b>source</b> folder contains <b>readme</b> file where all paths are explained how to use them.
 *
 * For example, if you want OTA, then you need BOOT mode option when checking readme.txt file.
 * In case you have 8Mbit flash (1 MByte = 512kB + 512kB) readme says this:
 *
\verbatim
***********************BOOT MODE***********************
download:
Flash size 8Mbit: 512KB+512KB
boot_v1.2+.bin          0x00000                      For boot operation. If there is boot_1.x where x is greater than 2, use that version (always newest)
user1.1024.new.2.bin    0x01000                      Exact program where AT commands are stored 
esp_init_data_default.bin   0xfc000 (optional)       Default data for configration. Use only in case you want to update software and restore your saved settings
blank.bin               0x7e000 & 0xfe000            blank (all zeros) for some flash sections for proper operation.
\endverbatim
 *
 * \par             Step 4: Configuring FLASH TOOL
 *
 * Open FLASH tool and insert paths into program and set locations described above.
 * After all is set, you should have on program something similar to image below.
 *
 * \image html update_process_step_4.png "Paths and locations are set and ready to update for BOOT mode on 8Mbit flash"
 * 
 * \par             Step 5: Start updating
 *
 * Before updating process will start, you must select COM PORT and BAUDRATE for UART.
 *
 * \note            Find COM port in device manager and set baudrate to 115200 bauds and press start.
 *
 * \note            If updating did not start, make sure COM port is correct and make sure you pull GPIO0 pin to LOW and you have reset device with RST pin to LOW and back HIGH.
 *
 * After you are ready, press <b>START</b> button to begin downloading software to ESP8266
 *
 * \image html update_process_step_5.png "Updating process in progress"
 *
 * \par             Step 6: Finish
 *
 * When download finishes, set GPIO0 pin back to HIGH and reset module again. Now you should have update module to new software.

 * \image html update_process_step_6.png "Updating process has finished"
 *
 * \image html update_process_step_6_test.png "Testing software on ESP8266"
 *
 */