
include <iec_mains.scad>
include <nuts.scad>

$fn = 100;

//  PCB

_x0 = 141.0;
_y0 = 55.2;

function yco(y) = pcb_dy - (y - _y0);
function xco(x) = x - _x0;

pcb_dx = xco(221.5);
pcb_dy = 107.2 - _y0;
pcb_dz = 1.5;
pcb_mount = 8; // height of mounting pillar
pcb_margin_x = 2.5;
pcb_margin_y = 13;
psu_dz = 15;

pcb_holes = [
    [   xco(144.5), yco(103.5) ],
    [   xco(212), yco(103.5) ],
    [   xco(212), yco(59) ],
    [   xco(144.5), yco(59) ],
];

// mains socket

outlet_dx = 86;
outlet_dy = 86;
outlet_dz = 4;
outlet_fixing_dx = 60.3;
outlet_fixing_r = m3_hole_r;
outlet_off = 0;
outlet_mount = pcb_mount - 2;

// IEC mains inlet
iec_style = iec_s3;

// switch
sw_r = 12.5/2;

ui_off_x = outlet_dx + 30;
ui_off_y = 15;

// LEDs
led_r = 5.2/2;
led_pitch = 11;
led_h = 8 - 2.5;
led_outer_r = 6/2;
led_upper = 2;

// Fan
// big fan
fan_dx = 25;
fan_dy = 25;
fan_dz = 7;
fan_indent = (fan_dx - 20) / 2;
fan_r = m3_hole_r;
fan_in_r = 16/2;
fan_out_r = 23.5/2;
fan_t = 20;
fan_blade_start = 0; // degrees
fan_blade_step = 20; // degrees
fan_blade_opening = 16; // degrees

// small fan
//fan_dx = 20;
//fan_dy = 20;
//fan_dz = 6.5;
//fan_indent = (20 - 16) / 2;
//fan_r = m2_hole_r;
//fan_in_r = 10/2;
//fan_out_r = 18/2;
//fan_t = 20;
//fan_blade_start = 15; // degrees
//fan_blade_step = 30; // degrees
//fan_blade_opening = 24; // degrees

// box
box_dx = 135;
box_dy = 100;
box_dz = 31;
box_thick = 3;
box_r = 5; // corner curve

lid_dz = 10;

// heatsink 

hs_dx = 25;
hs_dy = 16;
hs_dz = 23;
hs_hole_dz = 18;
hs_r = m2_hole_r;
hs_x0 = hs_dx + fan_dz - 1;
hs_y0 = box_dy - 36;
hs_z0 = hs_dz + 12;
hs_extra = 6;
hs_rot = [ 90, 180, 270 ];

// heatsink mount
hsm_dx = 15;
hsm_dy = 4; // mount thickness
hsm_dz = 22;
hsm_x0 = hs_x0 - (hs_dx/2);
hsm_y0 = hs_y0 + (hs_dx/2) +2;
hsm_z0 = box_dz;

// vent slots
vent_x0 = 7;
vent_pitch = 8;
vent_num = 5;

    /*
    *
    */

module fan_blades()
{
    translate( [ fan_dx/2, fan_dy/2, 0 ] )
    intersection()
    {
        // doughnut between fan inner and outer radius
        difference()
        {
            cylinder(h=fan_t, r=fan_out_r);
            cylinder(h=fan_t, r=fan_in_r);
        }
        d = fan_out_r * 1.2;
        // blades
        for (angle = [ fan_blade_start : fan_blade_step : 360 + fan_blade_start ] )
        {
            points = [
                [ 0, 0 ],
                [ d * sin(angle), d * cos(angle) ],
                [ d * sin(angle+fan_blade_opening), d * cos(angle+fan_blade_opening) ],
            ];
            linear_extrude(height=fan_t)
            polygon(points);
        }
    }
}

module fan()
{
    translate( [ -fan_dx/2, -fan_dy/2, 0 ] )
    //difference()
    {
        cube([ fan_dx, fan_dy, fan_dz ] );

        points = [
            [ fan_indent,           fan_indent ],
            [ fan_indent,           fan_dy - fan_indent ],
            [ fan_dx - fan_indent,  fan_dy - fan_indent ],
            [ fan_dx - fan_indent,  fan_indent ],
        ];

        for (xy = points)
        {
            translate([ xy[0], xy[1], 0 ])
            #cylinder(h = fan_t, r=fan_r); 
        }

        fan_blades();
    }
}

module vent()
{
    x = box_thick * 0.25;
    y = box_thick * 0.38;
    dz = box_dz * 0.7;

    points = [
        [ -1*x, -1*y ],
        [ -1*x, +3*y ],
        [ +3*x, +3*y ],
        [ +3*x, +1*y ],
        [ +1*x, +1*y ],
        [ +1*x, -3*y ],
        [ -3*x, -3*y ],
        [ -3*x, -1*y ],
        [ -1*x, -1*y ],
    ];

    translate( [ 0, 0, -dz/2 ] )
    linear_extrude(height=dz)
    polygon(points);
}

    /*
    *
    */

module outlet(extra)
{
    // see photo of diagram 14-Jan-2026
    a = 36;
    b = 22;
    c = 38;
    d = 18;
    e = 15;
    f = 54;
    g = 15;
    rx = 4.5;
    t = 20;
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

            hull()
            {
                translate([ ((outlet_dx-f)/2) + rx, e+(a/2), -t ])
                cylinder(h=t, r=rx);
                translate([ ((outlet_dx+f)/2) - rx, e+(a/2), -t ])
                cylinder(h=t, r=rx);

                translate([ (outlet_dx-c)/2, e+((a-g)/2), -t ])
                cube([ c, g, t] );
            }
        }

        // mounting holes
        union()
        {
            for (x = xs)
            {
                translate([ x, outlet_dy/2, -0.01-outlet_dz])
                cylinder(h=outlet_dz+0.01, r=outlet_fixing_r);
            }
        }
    }
}

    /*
    *
    */

module pcb()
{
    psu_dx = 35;
    psu_dy = 20;
    psu_x0 = xco(145.7) - 3.97;
    psu_y0 = yco(74.5) - 8.86;

    sw_x0 = xco(190);
    sw_y0 = yco(96);

    cut_size = 15;

    _led_x0 = xco(192.525) - 2.54;
    _led_y0 = yco(81.25);

    stm32_dx = 23;
    stm32_dy = 53;
    stm32_dz = 14;
    stm32_x0 = 38;
    stm32_y0 = 0;
    stm32_dz2 = 22;

    translate([ -pcb_dx/2, -pcb_dy/2, 0 ] )
    difference()
    {
        union()
        {
            cube([ pcb_dx, pcb_dy, pcb_dz ]);
            translate( [ psu_x0, psu_y0, pcb_dz ] )
            cube([ psu_dx, psu_dy, psu_dz ]);

            translate( [ stm32_x0, stm32_y0, pcb_dz ] )
            {
                cube([ stm32_dx, stm32_dy, stm32_dz ]);
                translate( [ 10, 40, 0 ] )
                cube([ 5, 5, stm32_dz2 ]);
            }

            // switch cutout
            translate([ sw_x0, sw_y0, -cut_size+0.01 ])
            cylinder(h=cut_size+pcb_dz, r=sw_r);

            // led cutout
            for (y = [ 0, 11 ] )
            {
                translate([ _led_x0, _led_y0 + y, -cut_size+0.01 ])
                cylinder(h=cut_size+pcb_dz, r=led_r);
            }
        }
        for (xy = pcb_holes)
        {
            translate([ xy[0], xy[1], -0.01 ])
            cylinder(h=pcb_dz+0.02, r=m3_hole_r);
        }
    }
}

    /*
    *
    */

module heatsink()
{
    t = 1;
    a = hs_dz * 0.25;
    rotate( [ 0, 0, 180 ] )
    translate([ -hs_dx/2, 0, 0 ] )
    difference()
    {
        union()
        {
            translate([ 0, a, 0 ] )
            cube([ hs_dx, t, hs_dz] );

            dx = hs_dx / 5;
            for (x = [ 0, dx, (2 * dx) - (t/2), (3 * dx) - (t/2), 4 * dx, hs_dx - t ] )
            {
                if ((x == 0) || (x == (hs_dx - t)))
                    translate([ x, 0, 0 ] )
                    cube([ t, hs_dy, hs_dz] );
                else
                    translate([ x, a, 0 ] )
                    cube([ t, hs_dy - a, hs_dz] );
            }
        }
        translate([ hs_dx/2, a-0.01, hs_hole_dz ] )
        rotate([ 270, 0, 0 ] )
        cylinder(h=t+0.02, r=hs_r);
    }
}

module heatsink_mount(extra)
{
    t = 1;
    translate([ -hsm_dx/2, 0, 0 ] )
    difference()
    {
        union()
        {
            cube([ hsm_dx, hsm_dy, hsm_dz + extra] );
        }
        translate([ hsm_dx/2, -0.01, hs_hole_dz + extra ] )
        rotate([ 270, 0, 0 ] )
        cylinder(h=hsm_dy+0.02, r=hs_r);
    }
}

    /*
    *
    */

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

    /*
    *
    */

module main()
{
    // outlet offsets
    ox0 = (outlet_dx - outlet_fixing_dx) / 2;
    oy0 = outlet_dy / 2;
    leds_extra = 5;

    x0 = box_dx - (pcb_dy/2) - pcb_margin_x;
    y0 = box_dy - (pcb_dx/2) - pcb_margin_y;

    difference()
    {
        union()
        {
            box(box_dz);

            // pcb mounting posts
            translate([ x0, y0, 0 ])
            {
                rotate( [ 0, 0, 270 ] )
                for (xy = pcb_holes)
                {
                    translate([ xy[0] - (pcb_dx/2), (pcb_dy/2) - xy[1], box_dz-pcb_mount-box_thick ])
                    cylinder(h=pcb_mount, r=m3_thread_r*2.5);
                }
            }
            // outlet mounting posts
            for (x = [ 0, outlet_fixing_dx ])
            {
                translate([ x + ox0, oy0, box_dz-box_thick-outlet_mount+0.01 ])
                cylinder(h=outlet_mount, r=m3_thread_r*3);
            }

            // ribs to strengthen panel
            rib_w = 5;
            dx = (outlet_dx - outlet_fixing_dx) / 2;
            rib_dz = box_dz - (outlet_mount + box_thick);
            rb_margin = box_r + 1;
            translate( [ 0, 0, rib_dz ] )
            for (x = [ dx, outlet_dx - dx ])
            {
                translate( [ x - (rib_w/2), -rb_margin/2, 0 ] )
                cube( [ rib_w, box_dy + rb_margin, outlet_mount ] );
            }
            translate( [ -rb_margin/2, outlet_dy - 6, rib_dz ] )
            cube( [ box_dx + rb_margin, rib_w, outlet_mount ] );

            // heatsink mount
            translate([ hsm_x0, hsm_y0, hsm_z0 ] )
            rotate([ 0, 180, 0 ] )
            heatsink_mount(hs_extra);
        }

        #union()
        {
            // pcb mounting posts
            translate([ x0, y0, 0 ])
            {
                rotate( [ 0, 0, 270 ] )
                for (xy = pcb_holes)
                {
                    translate([ xy[0] - (pcb_dx/2), (pcb_dy/2) - xy[1], box_dz-pcb_mount-box_thick ])
                    translate([ 0, 0, -0.01] )
                    cylinder(h=pcb_mount, r=m3_thread_r);
                }
            }

            fan_x0 = fan_dz;
            fan_y0 = outlet_dy - (fan_dy/2) - 8;
            fan_z0 = box_dz - (fan_dx / 2) - box_thick - 1;
            translate([ fan_x0, fan_y0, fan_z0 ] )
            rotate( [ 0, 270, 0 ] )
            fan();

            // make holes for ventilation ingress
            // side
            for (i = [ 0 : vent_num-1 ] )
            {
                translate([ box_dx + box_thick, vent_x0 + (i * vent_pitch), box_dz/2 ] )
                vent();
            }
            // front
            for (x = [ 10 : vent_pitch : box_dx-10 ] )
            {
                translate([ x, -box_thick, box_dz/2 ] )
                rotate([ 0, 0, 90 ] )
                vent();
            }

            translate([ outlet_off, outlet_off, box_dz+0.01 ] )
            outlet(15);

            translate([ outlet_off + (outlet_dx/2), box_dy - 15, (box_dz-box_thick)/2 ])
            rotate([ 90, 0, 180 ])
            iec_cutout(iec_style, 25, m3_hole_r);

            translate([ x0, y0, box_dz - pcb_mount - box_thick ])
            rotate([ 0, 0, 270 ] )
            rotate([ 180, 0, 0 ] )
            pcb();

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
            // outlet mounting posts
            for (x = [ 0, outlet_fixing_dx ])
            {
                for (x = [ 0, outlet_fixing_dx ])
                {
                    translate([ x+ox0, oy0, box_dz-box_thick-outlet_mount-0.01 ])
                    cylinder(h=outlet_mount+0.02, r=m3_thread_r);
                }
            }

            // heatsink 
            translate([ hs_x0, hs_y0, hs_z0 - box_dz ] )
            rotate(hs_rot)
            heatsink();
        }
    }
}

    /*
    *
    */

//intersection()
//{
if (1) rotate([ 0, 180, 0 ] )
translate( [ -box_dx/2, -box_dy/2, 0 ] )
{
    if (1)
        main();
    else
        lid();    
}
// pcb area
//translate( [ -75, -40, -40 ] )
//#cube([ 70, 80, 25 ]);
// fan area
//translate( [ 30, -4, -40 ] )
//#cube([ 50, 38, 45 ]);
//}

//devbox();
//rotate( [ 180, 0, 0 ] )
//devbox_lid();
if (0) difference()
{
    e = 2;
    union()
    {
        dx = led_pitch * 2;
        dy = 12;
        translate([ -led_pitch/2, -dy/2, 0 ] )
        cube([ dx, dy, e-0.2 ] );
        leds(e);
    }
    leds_cut(e);
}

// Test the fan / vent design

if (0)
rotate([ 0, 90, 0 ] )
difference()
{
    dx = 40;
    dy = 30;
    translate( [ -dx/2, -dy/2, 0 ] )
    cube ([ dx + (box_dz/2), dy, box_thick ] );
    translate([ 0, 0, -10 ] )
    fan();

    span = 20;
    for (y = [ -span : vent_pitch : span ] )
    {
        translate([ 22, y, box_thick/2 ] )
        rotate([ 0, 90, 0] )
        #vent();
    }
}

//  FIN
