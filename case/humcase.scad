// Case for storing humidity sensor
//
// Copyright (C) 2020  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

// Generate STL using OpenSCAD:
//   openscad humcase.scad -o humcase.stl

pcb_width = 28;
pcb_thick = 1.6;
pcb_length = 60;
batt_thick = 20;
esp_thick = 3.5;
extra_thick = 2;
wall_width = 2;
guide_diamenter = 2;
lid_height = 3;
lid_diameter = 2;
air_hole_diameter = 5;
slack = 0.5;
CUT = 0.01;

module lid_frame(width, thick) {
    taper = 3;
    module rpoint(x1, y1) {
        translate([x1, y1, lid_diameter/2])
            sphere(d=lid_diameter, $fs=.5);
    }
    module raw_lid() {
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
        dimple_diameter = 1;
        translate([-dimple_diameter, thick-taper-3, -CUT])
            cylinder(h=lid_height + 2*CUT, d=dimple_diameter, $fs=.1);
        translate([width+dimple_diameter, thick-taper-3, -CUT])
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
    translate([x, y, -CUT])
        cylinder(h=lid_height + 2*CUT, d=air_hole_diameter, $fs=.5);
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
            sphere(d=guide_diamenter, $fs=.5);
        translate([front_place, top_space + guide_space, z+wall_width])
            sphere(d=guide_diamenter, $fs=.5);
        translate([rear_place, top_space, z+wall_width])
            sphere(d=guide_diamenter, $fs=.5);
        translate([rear_place, top_space + guide_space, z+wall_width])
            sphere(d=guide_diamenter, $fs=.5);
    }
    module main_box() {
        box();
        pcb_guide(4);
        pcb_guide(15);
        pcb_guide(pcb_length-15);
        pcb_guide(pcb_length-4);
    }
    module air_hole(x, z) {
        translate([x+wall_width, -CUT, z+wall_width])
            rotate([-90, 0, 0])
                cylinder(h=wall_width + 2*CUT, d=air_hole_diameter, $fs=.5);
    }
    module air_holes(zlist) {
        for (z=zlist) {
            air_hole(4, z);
            air_hole(width/2, z);
            air_hole(width-4, z);
        }
    }
    difference() {
        main_box();
        air_holes([8, 18, 28, 38, 48]);
        battery_air_hole(wall_width + slack / 2 + 6, batt_hole_y);
        battery_air_hole(width + wall_width - slack / 2 - 6, batt_hole_y);
    }
}

//
// Object selection
//

case();
//lid();
