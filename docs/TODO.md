Possible future enhancements.

There is no support for this device. These enhancements may or may not
be implemented.

PCB
===

* Possibly route GPIO15 to GND to suppress rom boot debug messages.
  (Currently GPIO15 is used for battery measurements.)

Firmware
========

* Store user settings in a config file stored in flash, instead of
  configuring via "menuconfig". This would make it easier to configure
  and distribute the device (as a single binary image could be used
  for many devices).

* Support a "configuration mode" where the device starts in AP mode
  and allows a user to configure settings via an http server.

* The esp-idf mqtt implementation is not optimized for bulk uploads.
  It is thought that a custom mqtt implementation could notably reduce
  measurement upload time, which could extend battery charge.
