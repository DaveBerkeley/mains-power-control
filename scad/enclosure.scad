
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
pcb_dz = 1.5;
pcb_mount = 8; // height of mounting pillar
pcb_margin_x = 1;
pcb_margin_y = 6;
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

// switch
sw_r = 12.5/2;

ui_off_x = outlet_dx + 30;
ui_off_y = 15;

// box
box_dx = 170;
box_dy = 100;
box_dz = 30;
box_thick = 3;
box_r = 5; // corner curve

lid_dz = box_thick * 2;

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

module pcb()
{
    psu_dx = 35;
    psu_dy = 20;

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

    led_r = 5.2/2;
    led_pitch = 11;
    led_h = 8;

module leds(extra)
{
    // LEDs
    led_outer_r = 5.8/2;
    led_pcb_to_led = 5.4 - 1.5;
    led_pcb_dy = 10.5;
    led_pcb_t = 1.5;
    dx = led_pitch * 2;

    difference()
    {
        translate( [ -dx/4, -led_pcb_dy/2, 0 ] )
        cube( [ dx, led_pcb_dy, led_h + led_pcb_to_led - extra - 2 ] );

        for (x = [ 0, led_pitch ])
        {
            translate([ x, 0, -extra ])
            {
                cylinder(h=led_h+extra, r=led_r);
                translate( [ 0, 0, led_h ] )
                cylinder(h=led_pcb_to_led, r=led_outer_r);
            }
        }
    }
}

module leds_cut(extra)
{
    for (x = [ 0, led_pitch ])
    {
        translate([ x, 0, -extra ])
        {
            cylinder(h=extra, r=led_r);
        }
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
    // outlet offsets
    ox0 = (outlet_dx - outlet_fixing_dx) / 2;
    oy0 = outlet_dy / 2;
    leds_extra = 5;

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
                    cylinder(h=pcb_mount, r=m3_thread_r*2.5);
                    translate([ 0, 0, -0.01] )
                    cylinder(h=pcb_mount, r=m3_thread_r);
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
            translate( [ -rb_margin/2, outlet_dy - 10, rib_dz ] )
            cube( [ box_dx + rb_margin, rib_w, outlet_mount ] );

            // led mounts
            translate([ ui_off_x, ui_off_y, box_dz-leds_extra+0.01 ] )
            scale( [ 1, 1, -1 ] )
            leds(leds_extra);
        }

        #union()
        {
            translate([ ui_off_x, ui_off_y, box_dz-leds_extra+0.01 ] )
            scale( [ 1, 1, -1 ] )
            leds_cut(leds_extra);

            translate([ outlet_off, outlet_off, box_dz+0.01 ] )
            outlet(15);

            translate([ outlet_off + (outlet_dx/2), box_dy - 15, (box_dz-box_thick)/2 ])
            rotate([ 90, 0, 180 ])
            iec_cutout(iec_s3, 25, m3_hole_r);

            x0 = box_dx - (pcb_dx/2) - pcb_margin_x;
            y0 = box_dy - (pcb_dy/2) - pcb_margin_y;
            translate([ x0, y0, box_dz - pcb_mount - box_thick ])
            rotate([ 180, 0, 0 ] )
            pcb();

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
            // outlet mounting posts
            for (x = [ 0, outlet_fixing_dx ])
            {
                for (x = [ 0, outlet_fixing_dx ])
                {
                    translate([ x+ox0, oy0, box_dz-box_thick-outlet_mount-0.01 ])
                    cylinder(h=outlet_mount+0.02, r=m3_thread_r);
                }
            }

        }
    }
}

    /*
    * development box : to cover up the mains parts
    */

db_corner_r = m3_thread_r*2;
db_margin = (db_corner_r / 2) + 1;
db_t = 2;
db_mount = 8;
stm32_extra = 6;
db_dz = 12; // pcb_dz + db_mount + (db_t * 2) + psu_dz + 2;
db_xi = 62 - db_margin; // 2 * pcb_dx / 3;
db_xx = [ -db_margin, db_xi, pcb_dx + (2 * db_margin) + db_corner_r ];
db_yy = [ -db_margin, pcb_dy + (2 * db_margin) + db_corner_r + stm32_extra ];
db_lid_dz = 18;

module _devbox(h, r, short)
{
    ix_max = short ? 1 : 2;
    hull()
    for (ix = [ 0 : ix_max ])
    {
        x = db_xx[ix];
        for (y = db_yy)
        {
            translate([ x, y, 0] )
            cylinder(h=h, r=r);
        }
    }
}

module devbox()
{
    pcb_hole_off = db_margin;
    difference()
    {
        union()
        {
            // pcb mounts
            for (xy = pcb_holes)
            {
                translate([ xy[0] + pcb_hole_off, xy[1] + pcb_hole_off, 0] )
                {
                    difference()
                    {
                        cylinder(h=db_mount, r=m3_hole_r*2);
                        translate( [ 0, 0, +0.01 ] )
                        cylinder(h=db_mount+0.02, r=m3_thread_r);
                    }
                }
            }

            // base / sides
            difference()
            {
                _devbox(db_dz, db_corner_r, false);
                translate( [ 0, 0, db_t ] )
                _devbox(db_dz, 1, false);
                //translate( [ -db_margin, -db_margin, db_t ] )
                //cube([ pcb_dx + (db_margin*2), pcb_dy + (db_margin+2), db_dz ]);
            }

            // corner mounts
            for (x = db_xx)
            {
                for (y = db_yy)
                {
                    translate([ x, y, 0] )
                    cylinder(h=db_dz, r=db_corner_r);
                }
            }
        }

        // subtract these :

        translate( [ (pcb_dx/2) + db_margin, (pcb_dy/2) + db_margin, db_mount ] )
        #pcb();

        // pcb mounts
        for (xy = pcb_holes)
        {
            translate([ xy[0] + pcb_hole_off, xy[1] + pcb_hole_off, 0.01] )
            cylinder(h=db_mount+0.02, r=m3_thread_r);
        }

        // corner mounts
        for (x = db_xx)
        {
            for (y = db_yy)
            {
                translate([ x, y, db_t ] )
                cylinder(h=db_dz+0.02, r=m3_thread_r);
            }
        }
    }
}

module devbox_lid()
{
    // lid
    translate([ 0, 0, db_dz + 2 ])
    difference()
    {
        union()
        {
            // top
            translate([ 0, 0,  db_lid_dz ])
            _devbox(db_t, db_corner_r, true);

            // sides
            difference()
            {
                _devbox(db_lid_dz, db_corner_r, true);
                translate( [ 0, 0, -0.01 ] )
                _devbox(db_lid_dz, 1, true);
            }

            // mounts
            for (idx = [ 0 : 1 ])
            {
                x = db_xx[idx];
                for (y = db_yy)
                {
                    translate([ x, y, 0 ] )
                    cylinder(h=db_t + db_lid_dz + 0.02, r=db_corner_r+1);
                }                
            }
        }

        // holes for fixing screws
        for (idx = [ 0 : 1 ])
        {
            x = db_xx[idx];
            for (y = db_yy)
            {
                translate([ x, y, -0.01 ] )
                {
                    cylinder(h=db_t + db_lid_dz + 0.04, r=m3_hole_r);
                    db_head = 10;
                    translate( [ 0, 0, db_lid_dz + db_t - db_head + 0.01 ] )
                    cylinder(h=db_head + 0.04, r=m3_head_r);
                }
            }
        }

        // cutout for stm32
        {
            dx = db_t * 2;
            dy = pcb_dy + 2;
            dz = db_lid_dz;
            translate( [ db_xx[1], 2, -0.01 ] )
            cube( [ dx, dy, dz ] );
        }

        // cutout for mains power
        {
            mdx = db_t * 2;
            mdy = pcb_dy * 0.6;
            mdz = db_lid_dz * 0.4;
            translate( [ -db_margin*2 - db_t, 2, -0.01 ] )
            cube( [ mdx, mdy, mdz ] );
        }
    }
}

    /*
    *
    */

//intersection()
{
// base
//big = box_dx + 20;
//translate( [ -big/2, -big/2, -box_dz+2 ] )
//cube([ big, big, 2 ] );

// switches / leds
//translate([ -70, -50, -box_dz+2 ] )
//cube([ 50, 25, 2 ] );

if (1) rotate([ 0, 180, 0 ] )
translate( [ -box_dx/2, -box_dy/2, 0 ] )
{
    if (1)
        main();
    else
        lid();    
}
}

//devbox();
//rotate( [ 180, 0, 0 ] )
//devbox_lid();

//leds(2);

//  FIN
