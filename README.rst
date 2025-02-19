Simba
=====

This is the source code repository of the Simba Embedded Programming
Platform. See http://simba-os.readthedocs.org for further details.

Simba is written in C.

Don't hesitate to create issues or pull requests if you want to
improve Simba!

*“We don't make mistakes, we just have happy accidents.”*
― Bob Ross

Try it out!
===========

#. Download the `Arduino IDE`_ and install Simba using the Boards Manager.

   .. code-block:: text

      https://raw.githubusercontent.com/eerimoq/simba-releases/master/arduino/avr/package_simba_avr_index.json
      https://raw.githubusercontent.com/eerimoq/simba-releases/master/arduino/sam/package_simba_sam_index.json
      https://raw.githubusercontent.com/eerimoq/simba-releases/master/arduino/esp/package_simba_esp_index.json
      https://raw.githubusercontent.com/eerimoq/simba-releases/master/arduino/esp32/package_simba_esp32_index.json

#. Select a Simba board.
#. Open the blink example.
#. Upload!

See the `Simba installation documentation`_ for detailed step-by-step instructions.

Need help?
==========

#. Have a look at the `Simba documentation`_. It's actually pretty good. =)

#. Write an issue here on GitHub.


Contributing
============

Contributing is very easy, using GitHub for pull requests and code
review. Just follow the steps below for a smooth process.

#. Fork this repository.

#. Implement the new feature, bug fix or other improvement, usually on
   the master branch.

#. Implement test case(s) to ensure that future changes do not break
   legacy.

#. Create a pull request and wait for the code to be reviewed, usually
   done within a day or two.


License
=======

Simba is licensed under the MIT License. See LICENSE for a copy of the
licence. Third party source code and libraries that Simba depends on
may have other licenses. Most third party code is placed in the
``3pp`` folder.

.. _Arduino IDE: https://www.arduino.cc/en/Main/Software
.. _Simba installation documentation: http://simba-os.readthedocs.io/en/latest/getting-started.html#arduino-arduino-ide
.. _Simba documentation: http://simba-os.readthedocs.io/en/latest
