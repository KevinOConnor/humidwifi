The PCB design files can be found in the [pcb/](../pcb/) directory.
The boards were desigend using [KiCad](https://kicad-pcb.org/) and
manufactured using [JLC PCB](https://jlcpcb.com/).

The JLC PCB service was also used to assemble most of the surface
mounted parts. However, as of this writing (20200501), JLC does not
support assembly of the esp32-wroom chip nor the bme280 chip. These
two chips were manually added to the board using solder paste and a
hot air rework station. Attaching the esp32 chip with these tools was
not difficult, but successfully attaching the bme280 chip was
challenging.

The board is designed to use a 3.2V LiFePO4 battery. The device will
not function properly with a 3.7V Lithium battery nor with a regular
alkaline AA battery - inserting an incorrect battery is likely to
permanently damage the board and it may result in a fire hazard.

Case
====

The design for a plastic case can be found in the [case/](../case/)
directory. It can be printed using a 3d-printer. The case was designed
using [OpenSCAD](https://www.openscad.org/).
