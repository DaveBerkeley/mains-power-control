#!/bin/env python

import math

def fn(x):
    # power curve : integral of sin squared
    return ((0.5 * x) - (0.25 * math.sin(x*2))) / (math.pi / 2)

def scale(x):
    # values for counter timer device on stm32
    max_range = 11000
    min_range = 1000
    return int(min_range + ((max_range - min_range) * (1 - x)))

curve = [ 0 ] * 100

i = 0
while i < math.pi:
    x = i / math.pi
    percent = int(fn(i) * 100)
    s = scale(x)
    curve[percent] = s
    i += 0.001

print("const int power_lut[100] = {")

for i, v in enumerate(curve):
    if i == 0:
        v = 0 # force the triac off for percent==0
    if (i % 5) == 0:
        print("    ", end="")
    end = " "
    if (i % 5) == 4:
        end = "\n"
    print(f"{v},", end=end)

print("};")

# FIN
