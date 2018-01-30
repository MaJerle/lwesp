/**
 * \page            page_update_at_software Update ESP8266 AT software
 * \tableofcontents
 *
 * This section will describe how to update ESP8266 AT commands software to latest version,
 * which is provided from Espressif Systems and is still under development process to be stable as much as possible.
 *
 * \section         sect_update_process_preparations Before you start
 *
 * ESP8266 device is a microcontroller itself which supports like any other MCU normal program custom written by yourself
 * or any other source who has done this job. AT commands software is a "project" which has implemented 
 * AT commands for ESP8266 module and is developed by Espressif Systems.
 *
 * When you bought module on ebay (or any other source) you probably got AT software provided by Thinker.
 * Thinker is a company which made famous ESP8266 modules ESP-01 to ESP-12 and are all
 * available on Google search for review if needed.
 *
 * Default software from Thinker works OK when you trying to communicate with module using terminal on your computer.
 * I entered into big problems in development process because when communication was made with microcontroller,
 * software just didn't work because of very unstable AT software provided by Thinker.
 *
 * \note            If you want to know, if your ESP8266 runs on Thinker AT software,
 *                  you will see "Thinker" text on terminal if you reset module. More about that later.
 *
 * Espressif Systems has its own SDK (Software development kit) and they also provide AT commands program build on this SDK.
 * 
 * This software is still under development process to be very stable, but at the moment (at the time of writing this, AT 0.60 is out) 
 * software works really great without any serious problems (at least I didn't entered into).
 * 
 * \section         sect_update_process_hardware Hardware requirements for software
 *
 * Espressif Systems provide 2 AT version releases when new version comes out:
 *
 * - AT release with OTA (at least <b>8Mbit</b> flash memory)
 * - AT release without OTA (at least <b>4Mbit</b> flash memory)
 *
 * \par             AT with OTA (BOOT mode)
 *
 * OTA enables you to run <b>AT+CIUPDATE</b> command and module will try to connect to Espressif servers and download newest
 * software for AT commands if it exists. It if does, it will burn it to flash and from there, each new reset will start on 
 * new software.
 *
 * This option requires dual-bank flash with at least 8Mbit of flash memory which is included on every ESP8266 module you buy on eBay.
 * If your module does not have at least 8Mbit flash memory, you can't use OTA and AT+CIUPDATE will not be enabled in your program.
 *
 * Newest ESP8266 modules has 8Mbit flash included, however, when I first time bought ESP8266 module, it didn't have 8, but only 4Mbit module.
 * So I bought another modules (ESP--07) and now I have 8Mbit flash memory on all of them. I have also bought compatible flash with 32Mbit and
 * replaced them on older modules so now I have on all modules OTA installed.
 *
 * \par AT without OTA (Non BOOT mode)
 *
 * AT commands software without OTA requried 4Mbit flash memory and basically works on any ESP module, even if bought at very beginning
 * when these modules were first time sold.
 *
 * \section         sect_update_process_download Download AT software
 *
 * You can always get latest NON_OS SDK from Espressif Official website.
 * Latest version when writing this tutorial is NONOS_SDK 2.1 and is available <a href="http://espressif.com/en/support/download/sdks-demos"><b>here</b></a>.
 *
 * \note            SDK also includes compiled AT project inside <b>bin</b> directory in package.
 *
 * \note            Before you start with update process, always make sure you have latest version downloaded.
 *
 * \section         sect_update_process_download_flashtool Download flash tool
 *
 * Espressif provides download flash tool, available on website in section
 * <a href="http://espressif.com/en/support/download/other-tools"><b>other tools</b></a>.
 *
 * \section         sect_update_process_hardware_preparation Prepare ESP8266 module
 *
 * Before you can start updating software on ESP8266 module, make sure you have correct wiring.
 * For proper wiring, you should follow Google. I have ESP-01 and ESP-07 (also works with ESP-12) modules tested and for normal operation,
 * using wiring below:
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
\verbatim
GPIO0   GPIO2   GPIO15     BOOT MODE

LOW     HIGH    LOW        Serial programming mode, ready to update
HIGH    HIGH    LOW        Boot from flash (normal operation for ESP-01 to ESP-12 modules)
ANY     ANY     HIGH       BOot from SDCARD (if connected to SDIO port)
\endverbatim
 *
 * \note    To start update process, connect ESP8266 module with USB<->UART converter and set <b>GPIO0 pin LOW</b> and set RST pin low and back high to reset module.
 *
 * \section         sect_update_process_start Start updating process
 *
 * Let's start with updating our ESP8266 device with latest version. I will go step-by-step and try to cover all steps and possible problems. Let's go!
 *
 * \par             Step 1: Prepare software
 *
 * We have downloaded AT software and flash download tool. Now it's time to extract sources.
 * I used <b>D:/TUTORIAL</b> on my Windows as a root folder for updating ESP8266 module. You can use the same or use any other.
 *
 * Extract both files into that folder. 
 *
 * \image html update_process_step_1.png "Folder structure after extracting both files into root directory"
 *
 * \par             Step 2: Configure flash tool
 *
 * In folder "FLASH_DOWNLOAD..." is .exe. Open it and go back to root directory and then to "at" directory.
 *
 * \image html update_process_step_2.png "AT directory with opened flash download tool"
 *
 * \par             Step 3: Set paths and locations for uploading
 *
 * In "at" folder is a <b>readme.txt</b> where you will find which file should be used on which location.
 *
 * For example, if you want OTA, then you need BOOT mode option when checking readme.txt file.
 * In case you have 8Mbit flash (1 MByte = 512kB + 512kB) readme says this:
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
 * As you maybe can see, some files are missing. This is because we have to download SDK too to get missing files.
 * Latest SDK (time of writing this) was SDK 1.5.2 and is available <a href="http://bbs.espressif.com/viewtopic.php?f=46&t=1702" target="_new"><b>here</b></a> for download.
 *
 * Download SDK and inside zip file go to "esp_sdk_xxx/bin/" folder and extract all ".bin" files into your "at" directory you previously extracted on first step.
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