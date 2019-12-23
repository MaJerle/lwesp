.. _api_esp_sys:

System functions
================

System functions are bridge between operating system system calls and middleware system calls.
Middleware is tightly coupled with operating system features hence it is important to include OS features directly.

It includes support for:

* Thread management, to start/stop threads
* Mutex management
* Semaphore management, works only with binary semaphores
* Message queues for thread-safe data exchange between threads
* Core system protection for mutual exclusion to access shared resources

.. tip::
	Check :ref:`um_porting_guide` for actual implementation

.. doxygengroup:: ESP_SYS