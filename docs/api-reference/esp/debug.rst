.. _api_esp_debug:

Debug support
=============

Middleware has extended debugging capabilities. 
These consist of different debugging levels and types of debug messages,
allowing to track and catch different types of warnings, severe problems or simply output messages
program flow messages (trace messages).

Module is highly configurable using library configuration methods.
Application must enable some options to decide what type of messages and for which modules it would like to output messages.

With default configuration, ``printf`` is used as output function.
This behavior can be changed with :c:macro:`ESP_CF_DBG_OUT` configuration.

For successful debugging, application must:

* Enable global debugging by setting :c:macro:`ESP_CFG_DBG` to :c:macro:`ESP_DBG_ON`
* Configure which types of messages to output
* Configure debugging level, from all messages to severe only
* Enable specific modules to debug, by setting its configuration value to :c:macro:`ESP_DBG_ON`

.. tip::
    Check :ref:`api_esp_config` for all modules with debug implementation.

An example code with config and latter usage:

.. literalinclude:: ../../examples_src/debug_config.h
    :language: c
    :linenos:
    :caption: Debug configuration setup

.. literalinclude:: ../../examples_src/debug.c
    :language: c
    :linenos:
    :caption: Debug usage within middleware

.. doxygengroup:: ESP_DEBUG