
include <nuts.scad>

$fn = 100;

pitch = 10;
hole_r = m3_hole_r;
r = hole_r + 3;
t = 2;
xs = [ 0, pitch ];

difference()
{
    hull()
    {
        for (x = xs)
        {
            translate([ x, 0, 0 ] )
            cylinder(h=t, r=r);
        }
    }
    for (x = xs)
    {
        translate([ x, 0, -0.01 ] )
        cylinder(h=t+0.02, r=hole_r);
    }
}

