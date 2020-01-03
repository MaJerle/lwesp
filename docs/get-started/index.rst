.. _getting_started:

Getting started
===============

.. _download_library:

Download library
^^^^^^^^^^^^^^^^

Library is primarly hosted on `Github <https://github.com/MaJerle/esp-at-lib>`_.

* Download latest release from `releases area <https://github.com/MaJerle/esp-at-lib/releases>`_ on Github
* Clone `develop` branch for latest development

Download from releases
**********************

All releases are available on Github `releases area <https://github.com/MaJerle/esp-at-lib/releases>`_.

Clone from Github
*****************

First-time clone
""""""""""""""""

* Download and install ``git`` if not already
* Open console and navigate to path in the system to clone repository to. Use command ``cd your_path``
* Clone repository with one of available ``3`` options

  * Run ``git clone --recurse-submodules https://github.com/MaJerle/esp-at-lib`` command to clone entire repository, including submodules
  * Run ``git clone --recurse-submodules --branch develop https://github.com/MaJerle/esp-at-lib`` to clone `development` branch, including submodules
  * Run ``git clone --recurse-submodules --branch master https://github.com/MaJerle/esp-at-lib`` to clone `latest stable` branch, including submodules

* Navigate to ``examples`` directory and run favourite example

Update cloned to latest version
"""""""""""""""""""""""""""""""

* Open console and navigate to path in the system where your resources repository is. Use command ``cd your_path``
* Run ``git pull origin master --recurse-submodules`` command to pull latest changes and to fetch latest changes from submodules
* Run ``git submodule foreach git pull origin master`` to update & merge all submodules

.. note::
	This is preferred option to use when you want to evaluate library and run prepared examples.
	Repository consists of multiple submodules which can be automatically downloaded when cloning and pulling changes from root repository.

Add library to project
^^^^^^^^^^^^^^^^^^^^^^

At this point it is assumed that you have successfully download library, either cloned it or from releases page.

* Copy ``esp_at_lib`` folder to your project
* Add ``esp_at_lib/src/include`` folder to *include path* of your toolchain
* Add folder to port ``esp_at_lib/src/include/system/port/_arch_`` folder to *include path* of your toolchain
* Add source files from ``esp_at_lib/src/`` folder to toolchain build
* Add source files from ``esp_at_lib/src/system/`` folder to toolchain build for arch port
* Copy ``esp_at_lib/src/include/esp/esp_config_template.h`` to project folder and rename it to ``esp_config.h``
* Build the project

Configuration file
^^^^^^^^^^^^^^^^^^

Library comes with template config file, which can be modified according to needs.
This file shall be named ``esp_config.h`` and its default template looks like the one below:

.. tip::
    Check :ref:`api_esp_config` section for possible configuration settings

.. literalinclude:: ../../esp_at_lib/src/include/esp/esp_config_template.h
    :language: c
    :linenos:
    :caption: Config file template