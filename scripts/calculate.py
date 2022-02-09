#!/usr/bin/env python
# Script to calculate a voltage scale (for use in idf.py menuconfig)
#
# Copyright (C) 2022  Brian O'Connor <brian@btoconnor.net>
#
# This file may be distributed under the terms of the GNU GPLv3 license.
import optparse

def calculate(orig_voltage, actual_voltage, orig_scale, orig_offset):
    raw_measure = (orig_voltage - orig_offset) / orig_scale
    new_scale = (actual_voltage - orig_offset) / raw_measure
    return new_scale

def main():
    usage = "%prog <reported_voltage> <actual_voltage> [<scale> [<offset>]]"
    opts = optparse.OptionParser(usage)
    options, args = opts.parse_args()
    if len(args) < 2 or len(args) > 4:
        opts.error("Incorrect number of arguments")
    orig_voltage = float(args[0])
    actual_voltage = float(args[1])
    orig_scale = 2.0
    orig_offset = 0.089
    if len(args) > 2:
        orig_scale = float(args[2])
        if len(args) > 3:
            orig_offset = float(args[3])
    new_scale = calculate(orig_voltage, actual_voltage, orig_scale, orig_offset)
    print("New scale=%.3f" % new_scale)

if __name__ == '__main__':
    main()
