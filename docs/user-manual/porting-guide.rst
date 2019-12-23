.. _um_porting_guide:

Porting guide
=============

High level of *ESP-AT* library is platform independent, written in ANSI C99,
however there is an important part where middleware needs to communicate with target *ESP* device
and it must work under different optional operating systems selected by final customer.

Porting consists of:

* Implementation of *low-level* part, for actual communication between host device and *ESP* device
* Implementation of system functions, link between target operating system and middleware functions
* Assignment of memory for allocation manager

Implement low-level part
^^^^^^^^^^^^^^^^^^^^^^^^

Implement system functions
^^^^^^^^^^^^^^^^^^^^^^^^^^

Example: Low-level driver for WIN32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code for low-level porting on `WIN32` platform.
It uses native *Windows* features to open *COM* port and read/write from/to it.

Notes:

* It uses separate thread for received data processing.
  It uses :cpp:func:`esp_input_process` or :cpp:func:`esp_input` functions, based on application configuration of :c:macro:`ESP_CFG_INPUT_USE_PROCESS` parameter.

  * When :c:macro:`ESP_CFG_INPUT_USE_PROCESS` is disabled, dedicated receive buffer is created by *ESP-AT* library
    and :cpp:func:`esp_input` function just writes data to it and does not process received characters immediately.
    This is handled by *Processing* thread at later stage instead.
  * When :c:macro:`ESP_CFG_INPUT_USE_PROCESS` is enabled, :cpp:func:`esp_input_process` is used,
    which directly processes input data and sends potential callback/event functions to application layer.

* Memory manager has been assigned to ``1`` region of ``ESP_MEM_SIZE`` size
* It sets *send* and *reset* callback functions for *ESP-AT* library

.. literalinclude:: ../../esp_at_lib/src/system/esp_ll_win32.c
    :language: c
    :caption: Actual implementation of low-level driver for WIN32

Example: Low-level driver for STM32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code for low-level porting on `STM32` platform.
It uses `CMSIS-OS` based application layer functions for implementing threads & other OS dependent features.

Notes:

* It uses separate thread for received data processing.
  It uses :cpp:func:`esp_input_process` function to directly process received data without using intermediate receive buffer
* Memory manager has been assigned to ``1`` region of ``ESP_MEM_SIZE`` size
* It sets *send* and *reset* callback functions for *ESP-AT* library

.. literalinclude:: ../../esp_at_lib/src/system/esp_ll_stm32.c
    :language: c
    :caption: Actual implementation of low-level driver for STM32

Example: System functions for WIN32
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../esp_at_lib/src/include/system/esp_sys_win32.h
    :language: c
    :caption: Actual header implementation of system functions for WIN32
  
.. literalinclude:: ../../esp_at_lib/src/system/esp_sys_win32.c
    :language: c
    :caption: Actual implementation of system functions for WIN32

Example: System functions for CMSIS-OS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. literalinclude:: ../../esp_at_lib/src/include/system/esp_sys_cmsis_os.h
    :language: c
    :caption: Actual header implementation of system functions for CMSIS-OS based operating systems

.. literalinclude:: ../../esp_at_lib/src/system/esp_sys_cmsis_os.c
    :language: c
    :caption: Actual implementation of system functions for CMSIS-OS based operating systems

.. toctree::
    :maxdepth: 2
    :glob: