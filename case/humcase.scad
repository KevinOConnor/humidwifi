// Case for storing humidity sensor
//
// Copyright (C) 2020-2021  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

// Generate STL using OpenSCAD:
//   openscad humcase.scad -o humcase.stl

pcb_width = 28;
pcb_thick = 1.6;
pcb_length = 60;
batt_thick = 18.5;
esp_thick = 3.5;
extra_thick = 0;
wall_width = 2;
guide_diamenter = 2;
lid_height = 3;
lid_diameter = 2;
air_hole_diameter = 3.5;
slack = 0.5;
CUT = 0.01;
$fs = 0.5;

module lid_frame(width, thick) {
    taper = 3;
    module raw_lid() {
        module rpoint(x1, y1) {
            translate([x1, y1, lid_diameter/2])
                sphere(d=lid_diameter);
        }
        hull() {
            rpoint(0, 0);
            rpoint(0, thick-taper);
            rpoint(taper, thick);
            rpoint(width-taper, thick);
            rpoint(width, thick-taper);
            rpoint(width, 0);
        }
        linear_extrude(height=lid_height) {
            polygon([[0,0], [0, thick-taper], [taper, thick],
                     [width-taper, thick], [width, thick-taper], [width, 0]]);
        }
    }
    module dimples() {
        dimple_diameter = 1.5;
        x_offset = lid_diameter/2;
        y_pos = thick - taper - 3;
        translate([-x_offset, y_pos, -CUT])
            cylinder(h=lid_height + 2*CUT, d=dimple_diameter, $fs=.1);
        translate([width + x_offset, y_pos, -CUT])
            cylinder(h=lid_height + 2*CUT, d=dimple_diameter, $fs=.1);
    }
    difference() {
        raw_lid();
        translate([-lid_diameter/2 - CUT, -lid_diameter, 0])
            cube([width + lid_diameter + 2*CUT, lid_diameter,
                  lid_height + 2*CUT]);
        dimples();
    }
}

module battery_air_hole(x, y) {
    batt_air_hole_diameter = 5;
    translate([x, y, -CUT])
        cylinder(h=lid_height + 2*CUT, d=batt_air_hole_diameter);
}

batt_hole_y = extra_thick + esp_thick + pcb_thick + wall_width + batt_thick/2;

module lid() {
    difference() {
        lid_frame(pcb_width, batt_thick + esp_thick + extra_thick + wall_width);
        battery_air_hole(6, batt_hole_y);
        battery_air_hole(pcb_width-6, batt_hole_y);
    }
}

module case() {
    width = pcb_width + slack;
    thick = batt_thick + esp_thick + extra_thick + slack;
    height = pcb_length + lid_height + slack;
    two_wall = 2 * wall_width;
    module box() {
        difference() {
            cube([width + two_wall, thick + two_wall, height + wall_width]);
            translate([wall_width, wall_width, wall_width])
                cube([width, thick, height - lid_height + 2*CUT]);
            translate([wall_width, -CUT, wall_width + pcb_length + slack + CUT])
                lid_frame(width, thick + wall_width);
        }
    }
    module pcb_guide(z) {
        top_space = esp_thick + wall_width + extra_thick;
        guide_space = pcb_thick + guide_diamenter + slack;
        front_place = wall_width;
        rear_place = wall_width + width;
        translate([front_place, top_space, z+wall_width])
            sphere(d=guide_diamenter);
        translate([front_place, top_space + guide_space, z+wall_width])
            sphere(d=guide_diamenter);
        translate([rear_place, top_space, z+wall_width])
            sphere(d=guide_diamenter);
        translate([rear_place, top_space + guide_space, z+wall_width])
            sphere(d=guide_diamenter);
    }
    module main_box() {
        box();
        pcb_guide(4);
        pcb_guide(15);
        pcb_guide(pcb_length-15);
        pcb_guide(pcb_length-4);
    }
    module air_holes(xpos_list, zpos_list) {
        module air_hole(x, z) {
            translate([x, -CUT, z])
                rotate([-90, 0, 0])
                    cylinder(h=wall_width + 2*CUT, d=air_hole_diameter);
        }
        center_x = (width + two_wall) / 2;
        center_z = (height + wall_width) / 2;
        gap = 2 * air_hole_diameter;
        for (xpos=xpos_list) {
            x = center_x + gap * xpos;
            for (zpos=zpos_list) {
                z = center_z + gap * zpos;
                air_hole(x, z);
            }
        }
    }
    difference() {
        main_box();
        air_xlist = [-1.5, -0.5, .5, 1.5];
        air_zlist = [-3.5, -2.5, -1.5, -0.5, .5, 1.5, 2.5, 3.5];
        air_holes(air_xlist, air_zlist);
        battery_air_hole(wall_width + slack / 2 + 6, batt_hole_y);
        battery_air_hole(width + wall_width - slack / 2 - 6, batt_hole_y);
    }
}

//
// Object selection
//

case();
//lid();
