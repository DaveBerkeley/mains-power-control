#!/usr/bin/env python3

import os

def make_link(src, dst, path):
    s = src + path
    d = dst + path
    d_dir = os.path.dirname(d)
    s_dir = os.path.dirname(s)
    _, name = os.path.split(path)

    d_path = os.path.join(dst, path)
    r, _ = os.path.split(d_path)
    rel = '../' * len(r.split('/'))
    s_path = os.path.join(rel, s_dir, name)

    #raise Exception((s_dir, name, s_path, path, d, rel))

    if not os.path.exists(d_dir):
        print('create dir', d_dir)
        os.makedirs(d_dir)

    if not os.path.lexists(d):
        print('create link', d)
        os.symlink(s_path, d)

#
#

files = [
    'list.cpp',
    'io.cpp',
    'logger.cpp',

    'panglos/debug.h',
    'panglos/mutex.h',
    'panglos/list.h',
    'panglos/io.h',
    'panglos/logger.h',

    'panglos/drivers/gpio.h',
    'panglos/drivers/uart.h',

    'panglos/stm32/gpio_f1.h',
    'panglos/stm32/uart_f1.h',

    'panglos/arch.h',
    'panglos/stm32/arch.h',

    'stm32/gpio_f1.cpp',
    'stm32/uart_f1.cpp',
]

for path in files:
    make_link('../lib/panglos/src/', 'lib/panglos/src/', path)

#
#   Put the google test simulation in its own lib

#files = [
#    'gtest/gtest.h',
#    'gtest.cpp',
#]

#for path in files:
#    make_link('../panglos/', 'lib/gtest/src/', path)

print("built library links ...")

# FIN
