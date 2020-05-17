Possible future enhancements.

There is no support for this device. These enhancements may or may not
be implemented.

PCB
===

* Improve the thermal isolation of the bme280 chip. It is believed
  that the battery may be acting as a "heat sink" which may limit the
  responsiveness of the bme280 temperature measurements. Ideally the
  bme280 chip would be moved to the edge of the board and would not
  have a ground fill near it. Also, the case should have vent holes
  for the battery side of the pcb.

* Add an SMT mounted push-button device to the board. A button could
  allow for improved user interaction during configuration and code
  updates.

* Route all the unused esp32 gpio pins to new 2.54mm pin headers. It's
  not difficult to route these pins and it may be interesting to
  attach external sensors to this simple battery powered esp32 board.

* Minor routing changes: Move the "3.2V lifepo4 warning" so that it
  can be seen after the battery holder is soldered on; possibly change
  the order of the debug pins to match some common esp32 debug devices
  (gnd, gpio0, vcc, tx, rx, rst); possibly route GPIO15 to GND to
  suppress rom boot debug messages.

Firmware
========

* Store user settings in a config file stored in flash, instead of
  configuring via "menuconfig". This would make it easier to configure
  and distribute the device (as a single binary image could be used
  for many devices).

* Support a "configuration mode" where the device starts in AP mode
  and allows a user to configure settings via an http server.
