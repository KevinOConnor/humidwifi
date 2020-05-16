There is custom firmware available for this PCB design in the
[fw/](../fw/) directory. This document provides some overview
information for the firmware.

Building the firmware
=====================

The code uses the
[esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)
framework. To build the firmware run:

```
cd /path/to/humidwifi/
docker run --rm -v ./fw:/project -w /project -it espressif/idf:release-v4.1 idf.py menuconfig
docker run --rm -v ./fw:/project -w /project -it espressif/idf:release-v4.1 idf.py build
```

In the menuconfig step, be sure to enter the `Application settings for
'humidity wifi' project` and configure the appropriate MQTT and
network settings. It is recommended to use a static IP address for the
device as this greatly reduces the time it takes for the device to
connect.

After configuring and building the software it will be necessary to
flash the software to the board. This can be done by running something
like `docker run --rm -v ./fw:/project -w /project -it
espressif/idf:release-v4.1 idf.py flash`, but it will require
configuring docker to have direct access to the device's serial port
(which is outside the scope of this document).

Overview and notes
==================

The software was designed to minimize battery usage during normal
operation. The goal is to keep the esp32 in "deep sleep" mode for the
vast majority of its lifetime. Sensor readings are taken periodically,
the results are stored to the esp32's "low power rtc memory", and then
the device reenters deep sleep mode. Periodically the device will also
turn on the radio, upload stored sensor readings, check for an
"over-the-air flash request", and then turn off the radio. Priority is
given to minimizing the amount of time that the wifi radio is enabled.
Minimizing the radio time greatly reduces the battery drain.

The software performs all uploads via an MQTT server. It should be
possible to configure the MQTT URI to use a username/password and/or
to use a TLS encrypted connection. The code does not currently have a
method to store TLS certificates however. The code does set the MQTT
"retain" flag, which may not be compatible with some "cloud" MQTT
servers. It should work with a local
[Mosquitto MQTT](https://mosquitto.org/) server.

Battery measurement
===================

The firmware uses an esp32 "trick" to measure the battery voltage. It
enables both the pullup and pulldown resistors of an unconnected ADC2
pin and then performs an ADC measurement. The pullup/pulldown
resistors form a "voltage divisor" which can be used to measure the
battery voltage. In theory, this "trick" would result in an ADC
reading that is half the battery voltage and therefore one would scale
this value by a factor of 2.0 to obtain the actual battery voltage.
However, the on-chip pullup/pulldown resistors may have slightly
different resistances.

The resistance difference is usually minor, but one can account for it
if they wish. While the device is powered and running, use a
multimeter to measure the actual voltage across the battery terminals.
Compare this voltage with the voltage reported by the software and
update the 'Voltage scale' parameter in the "menuconfig" tool
accordingly.

Over-the-air flash
==================

It is possible to flash new versions of the software using an "over
the air" update. This is done by writing a URL to the `topic/ota_url`
MQTT topic (where `topic` is the topic prefix specified in
menuconfig). The URL should be an http (or https) link. For example:

```
mosquitto_pub -t "topic/ota_url" -m "http://myserver:8080/mqtt_tcp.bin" -r
```

When the device next attempts to upload values, it will look for that
topic and then attempt to download the given firmware. One can launch
a temporary http server with something like:

```
cd /path/to/humidwifi/fw/build/
python -m SimpleHTTPServer 8080
```

It should be possible to use an https server, however the code does
not currently verify TLS certificates.
