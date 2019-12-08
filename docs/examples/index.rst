.. _examples:

Examples and demos
==================

Various examples are provided for fast library evaluation on embedded systems. These are optimized prepared and maintained for ``2`` platforms, but could be easily extended to more platforms:

* WIN32 examples, prepared as `Visual Studio Community <https://visualstudio.microsoft.com/vs/community/>`_ projects
* ARM Cortex-M examples for STM32, prepared as `STM32CubeIDE <https://www.st.com/en/development-tools/stm32cubeide.html>`_ GCC projects

.. warning::
	Library is platform independent and can be used on any platform.

Supported architectures
^^^^^^^^^^^^^^^^^^^^^^^

There are many platforms available today on a market, however supporting them all would be tough task for single person.
Therefore it has been decided to support (for purpose of examples) ``2`` platforms only, `WIN32` and `STM32`.

WIN32
*****

Examples for *WIN32* are prepared as `Visual Studio Community <https://visualstudio.microsoft.com/vs/community/>`_ projects.
You can directly open project in the IDE, compile & debug.

STM32
*****

Embedded market is supported by many vendors and STMicroelectronics is, with their `STM32 <https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html>`_ series of microcontrollers, one of the most important players.
There are numerous amount of examples and topics related to this architecture.

Examples for *STM32* are natively supported with `STM32CubeIDE <https://www.st.com/en/development-tools/stm32cubeide.html>`_, an official development IDE from STMicroelectronics.

You can run examples on one of official development boards, available in repository examples.

Examples list
^^^^^^^^^^^^^

Here is a list of all examples coming with this library.

.. tip::
	Examples are located in ``/examples/`` folder in downloaded package.
	Check :ref:`download_library` section to get your package.

LwMEM bare-metal
****************

Simple example, not using operating system, showing basic configuration of the library.
It can be also called `bare-metal` implementation for simple applications

LwMEM OS
********

LwMEM library integrated as application memory manager with operating system.
It configurex mutual exclusion object ``mutex`` to allow multiple application threads accessing to LwMEM core functions

.. toctree::
	:maxdepth: 2
