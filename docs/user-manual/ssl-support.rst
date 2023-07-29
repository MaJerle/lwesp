.. _um_ssl_support:

TCP connection SSL support
==========================

.. warning:: 
    SSL support is currently in experimental mode. API changes may occur in the future.

ESP-AT binary, running on Espressif chips, supports SLL connection types.
Such connections, to work properly, require client or server certificates to be loaded to Espressif device.

With the recent update, *July 29th, 2023*, library has been updated to support AT commands for flash and MFG operations,
allowing host microcontroller to load required certificates to the Espressif device.

.. note::
    Minimum required ESP-AT library running on ESP device is now ``v3.2.0``, which supports new *AT+SYSMFG* command,
    that is required to load custom data to the device.

.. note::
    SSL connections mentioned on this page are secure from Espressif device towards network.
    Data between host MCU and Espressif MCU is not protected and may be exposed to an attacker

Prepare the certificate
***********************

Assuming we would like to establish connection to another server with secure SSL connection,
ESP device shall have up to ``3`` certificates loaded in its own system flash. These are:

* *client_ca* - Client root CA certificate - client uses this certificate to verify server. `Example <https://github.com/espressif/esp-at/blob/c0391fd1/components/customized_partitions/raw_data/client_ca/client_ca_00.crt>`_
* *client_cert* - Client certificate. `Example <https://github.com/espressif/esp-at/blob/c0391fd1/components/customized_partitions/raw_data/client_cert/client_cert_00.crt>`_
* *client_key* - Client private key. `Example <https://github.com/espressif/esp-at/blob/c0391fd1/components/customized_partitions/raw_data/client_key/client_key_00.key>`_

ESP-AT website includes some test certificates, that could be used for test purposes: `Description of all slots <https://docs.espressif.com/projects/esp-at/en/latest/esp32/Compile_and_Develop/How_to_update_pki_config.html>`_

LwESP repository contains aforementioned certificates in the *certificates* folder. There are *2* files for each certificate:

* Original *.crt* or *.key* file
* Original *.crt* or *.key* file converted to *.hex* array for easier include in the C project.

Load to ESP device
******************

Loading can be done as part of custom AT firmware build, or by using AT commands.
LwESP library has the support for system flash and manufacturing NVS data operation, that is required for certificate load.

All combined, steps to establish SSL connection is to:

* Have certificates loaded to ESP-AT device with Espressif format
* Have configured connections to use your certificates, if *SSL* type is used on them
* Have valid time in ESP device. *SNTP* module can help with that

Example
*******

Below is the up-to-date netconn API example using SSL connection. Example file is located in ``snippets/netconn_client_ssl.c``

.. literalinclude:: ../../snippets/netconn_client_ssl.c
    :language: c
    :linenos:
    :caption: Netconn example with SSL

.. toctree::
    :maxdepth: 2
    :glob: