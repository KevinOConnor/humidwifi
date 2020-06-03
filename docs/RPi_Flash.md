This document provides some hints on flashing the esp32 chip using a
Raspberry Pi's GPIO pins.

Wiring
======

The PCB's GPIO pins (from esp32 chip to board edge) are:
`GND, Tx, Rx, Boot, Reset, 3.3V`

Start by removing the battery and then use female to female jumper
cables to connect the PCB pins to the Raspberry Pi's GPIO
pins. Connect ground first and connect 3.3V last.

Important! Do not connect the 3.3V line while the battery is
installed.

Make these connections:
```
GND   -> RPi GND  (Pin 6)
Tx    -> RPi Rx   (Pin 10)
Rx    -> RPi Tx   (Pin 8)
Boot  -> RPi GND  (Pin 14)
Reset -> Not connected
3.3V  -> RPi 3.3V (Pin 1)
```

See https://pinout.xyz/ for a picture showing the Raspberry Pi GPIO
pins.

Software
========

The easiest way to build and install the software is to install and
compile the full
[esp-idf](https://docs.espressif.com/projects/esp-idf/en/v4.1-beta1/get-started/index.html)
environment on the Raspberry Pi and run `idf.py menuconfig ; idf.py
build ; idf.py -p /dev/ttyAMA0 flash`.

As an alternative, it is also possible to install just the idf
flashing tool and copy the binary components from a build on a desktop
computer. To do this, first install the esp32 flash tool:
```
virtualenv ~/esphome-env
~/esptool-env/bin/pip install esptool
```

Then copy the firmware files from a desktop to the Raspberry Pi with
something like:
```
cd /path/to/humidwifi/
rsync -av fw/build/partition_table/partition-table.bin fw/build/bootloader/bootloader.bin fw/build/humidwifi.bin pi@raspberrypi:~/
```

If one wishes to first erase the flash (which is not normally needed)
then one can run the following on the Raspberry Pi:
```
~/esptool-env/bin/esptool.py -p /dev/ttyAMA0 erase_flash
```
After this completes (it will take 30 seconds or so to erase the
chip), reset the PCB by carefully unplugging the 3.3V pin from the
PCB, wait 5 seconds, and then reconnect the 3.3V wire.

One can program the new flash with:
```
~/esptool-env/bin/esptool.py -p /dev/ttyAMA0 write_flash 0x8000 partition-table.bin 0x1000 bootloader.bin 0x10000 humidwifi.bin
```

Once the new firmware is written, carefully unplug the 3.3V wire
followed by the remaining wires. Insert the battery to start the new
firmware.

Important! Do not install the battery while the the 3.3V line is
connected.
