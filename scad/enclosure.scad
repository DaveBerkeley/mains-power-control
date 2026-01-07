
include <iec_mains.scad>
include <nuts.scad>

$fn = 100;

//  PCB

_x0 = 141;
_y0 = 55.2;

function yco(y) = pcb_dy - (y - _y0);
function xco(x) = x - _x0;

pcb_dx = xco(221.5);
pcb_dy = 107.2 - _y0;
pcb_dz = 2;
pcb_mount = 6; // height of mounting pillar
pcb_margin_x = 1;
pcb_margin_y = 8;

pcb_holes = [
    [   xco(144.5), yco(103.5) ],
    [   xco(212), yco(103.5) ],
    [   xco(212), yco(59) ],
];

// mains socket

outlet_dx = 86;
outlet_dy = 86;
outlet_dz = 4;
outlet_fixing_dx = 60.3;
outlet_fixing_r = m3_hole_r;
outlet_off = 0;
outlet_mount = 10;

// LEDs
led_r = 5/2;
led_pitch = 11;

// switch
sw_r = 11.8/2;

ui_off_x = outlet_dx + 30;
ui_off_y = 15;

// box
box_dx = 170;
box_dy = 100;
box_dz = 30;
box_thick = 2;
box_r = 5; // corner curve

lid_dz = box_thick * 2;

module outlet(extra)
{
    a = 15;
    b = 25;
    c = 50;
    d = 20;
    e = 20;
    t = 30;
    //difference()
    {
        x_off = (outlet_dx - outlet_fixing_dx) / 2;
        xs = [ x_off, x_off + outlet_fixing_dx ];
        union()
        {
            cube( [ outlet_dx, outlet_dy, outlet_dz ] );

            // cut-out for the plug
            translate([ (outlet_dx-c)/2, e, -t ])
            cube( [ c, a, t ] );
            translate([ (outlet_dx-d)/2, e+a, -t ])
            cube( [ d, b, t ] );
        }

        // mounting holes
        union()
        {
            for (x = xs)
            {
                translate([ x, outlet_dy/2, -extra-0.01])
                cylinder(h=outlet_dz+extra+0.02, r=outlet_fixing_r);
            }
        }
    }
}

module pcb()
{
    psu_dx = 35;
    psu_dy = 20;
    psu_dz = 15;

    rotate([ 180, 0, 0 ] )
    translate([ -pcb_dx/2, -pcb_dy/2, 0 ] )
    difference()
    {
        union()
        {
            cube([ pcb_dx, pcb_dy, pcb_dz ]);
            translate( [ 0, pcb_dy - psu_dy, pcb_dz ] )
            cube([ psu_dx, psu_dy, psu_dz ]);
        }
        for (xy = pcb_holes)
        {
            translate([ xy[0], xy[1], -0.01 ])
            cylinder(h=pcb_dz+0.02, r=m3_hole_r);
        }
    }
}

module leds(extra)
{
    for (x = [ 0, led_pitch ])
    {
        translate([ x, 0, 0 ])
        cylinder(h=extra, r=led_r);
    }
}

module _box(r, dz)
{
    hull()
    for (x = [ 0, box_dx ])
    {
        for (y = [ 0, box_dy ])
        {
            translate([ x, y, 0 ] )
            cylinder(h=dz, r=r);
        }
    }
}

module box(dz)
{
    difference()
    {
        _box(box_r, dz);
        translate([ 0, 0, -box_thick ] )
        _box(1, dz);
    }

    // box corner fixings
    for (x = [ 0, box_dx])
    {
        for (y = [ 0, box_dy ] )
        {
            translate([ x, y, 0 ])
            cylinder(h=dz, r=box_r);
        }
    }
}

module lid()
{
    difference()
    {
        box(lid_dz);
        for (x = [ 0, box_dx])
        {
            for (y = [ 0, box_dy])
            {
                translate([ x, y, -0.01 ] )
                cylinder(h=lid_dz+0.02, r=m3_hole_r);
            }
        }
    }
}

module main()
{
    difference()
    {
        union()
        {
            box(box_dz);

            // pcb mounting posts
            for (xy = pcb_holes)
            {
                x0 = xy[0] + box_dx - pcb_dx - pcb_margin_x;
                y0 = -xy[1] + box_dy - pcb_margin_y;
                translate([ x0, y0, box_dz-pcb_mount-box_thick ])
                difference()
                {
                    cylinder(h=pcb_mount, r=m3_thread_r*3);
                    translate([ 0, 0, -0.01] )
                    cylinder(h=pcb_mount, r=m3_thread_r);
                }
            }
            // outlet mounting posts
            for (x = [ 0, outlet_fixing_dx ])
            {
                x0 = x + ((outlet_dx - outlet_fixing_dx) / 2);
                y0 = outlet_dy / 2;
                translate([ x0, y0, box_dz-box_thick-outlet_mount+0.01 ])
                cylinder(h=outlet_mount, r=m3_thread_r*3);
            }
        }

        #union()
        {
            translate([ outlet_off, outlet_off, box_dz+0.01 ] )
            outlet(15);

            translate([ outlet_off + (outlet_dx/2), box_dy - 5, (box_dz/2) ])
            rotate([ 90, 0, 180 ])
            iec_cutout(iec_s3, 20, m3_hole_r);

            x0 = box_dx - (pcb_dx/2) - pcb_margin_x;
            y0 = box_dy - (pcb_dy/2) - pcb_margin_y;
            translate([ x0, y0, box_dz - pcb_mount - pcb_dz ])
            pcb();

            translate([ ui_off_x, ui_off_y, box_dz-10 ] )
            leds(20);
            translate([ ui_off_x + 30, ui_off_y, box_dz-10 ] )
            cylinder(h=20, r=sw_r);

            // box corner fixings
            for (x = [ 0, box_dx])
            {
                for (y = [ 0, box_dy ] )
                {
                    translate([ x, y, 0 ])
                    translate([ 0, 0, -box_thick ] )
                    cylinder(h=box_dz, r=m3_thread_r);
                }
            }
        }
    }
}

rotate([ 0, 180, 0 ] )
translate( [ -box_dx/2, -box_dy/2, 0 ] )
{
    if (1)
        main();
    else
        lid();    
}

//  FIN
