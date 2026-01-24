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

    #print(s_dir, name, s_path, path, d, rel)
    #raise Exception((s_dir, name, s_path, path, d, rel))

    if d_dir and not os.path.exists(d_dir):
        print(f'create dir "{d_dir}"')
        os.makedirs(d_dir)

    if not os.path.lexists(d):
        print(f'create link "{d}"')
        assert os.path.exists(s), s
        os.symlink(s_path, d)

#
#

files = [
    'list.cpp',
    'io.cpp',
    'logger.cpp',
    'object.cpp',
    'json.cpp',
    'verbose.cpp',
    'watchdog.cpp',
    'device.cpp',
    'time.cpp',
    'network.cpp',
    'socket.cpp',
    'cli_net.cpp',
    'event_queue.cpp',

    'drivers/spi.cpp',
    'drivers/ds18b20.cpp',

    'esp32/hal.cpp',
    'esp32/storage.cpp',
    'esp32/network.cpp',
    'esp32/gpio.cpp',
    'esp32/mqtt.cpp',
    'esp32/uart.cpp',
    'esp32/rmt_strip.cpp',
    'esp32/timer.cpp',
    'esp32/one_wire_bitbang.cpp',

    'app/cli.cpp',
    'app/event.cpp',
    'app/cli_server.cpp',
    'app/cli_cmd.cpp',
    'app/devices.cpp',

    'freertos/mutex.cpp',
    'freertos/thread.cpp',
    'freertos/time.cpp',
    'freertos/semaphore.cpp',
    'freertos/queue.cpp',

    'riscv32/arch.cpp',

    'panglos/debug.h',
    'panglos/mutex.h',
    'panglos/list.h',
    'panglos/io.h',
    'panglos/logger.h',
    'panglos/network.h',
    'panglos/json.h',
    'panglos/mqtt.h',
    'panglos/storage.h',
    'panglos/object.h',
    'panglos/ring_buffer.h',
    'panglos/time.h',
    'panglos/verbose.h',
    'panglos/device.h',
    'panglos/arch.h',
    'panglos/thread.h',
    'panglos/event_queue.h',
    'panglos/watchdog.h',
    'panglos/hal.h',
    'panglos/json.h',
    'panglos/semaphore.h',
    'panglos/queue.h',
    'panglos/batch.h',
    'panglos/cli_net.h',
    'panglos/socket.h',
    'panglos/tx_net.h',
    'panglos/dispatch.h',
    'panglos/deque.h',

    'panglos/drivers/gpio.h',
    'panglos/drivers/uart.h',
    'panglos/drivers/timer.h',
    'panglos/drivers/led_strip.h',
    'panglos/drivers/one_wire.h',
    'panglos/drivers/temperature.h',
    'panglos/drivers/ds18b20.h',

    # lose these : 
    'panglos/drivers/mcp23s17.h',
    'panglos/drivers/i2c.h',
    'panglos/drivers/keyboard.h',
    'panglos/drivers/i2c_bitbang.h',
    'panglos/drivers/motor.h',
    'panglos/drivers/pwm.h',
    'panglos/drivers/7-segment.h',
    'panglos/drivers/rtc.h',
    'panglos/drivers/spi.h',

    'panglos/esp32/rmt_strip.h',
    'panglos/esp32/gpio.h',
    'panglos/esp32/timer.h',
    'panglos/esp32/hal.h',
    'panglos/esp32/uart.h',
    'panglos/esp32/i2c.h',

    'panglos/riscv32/arch.h',

    'panglos/app/event.h',
    'panglos/app/devices.h',
    'panglos/app/cli_server.h',
    'panglos/app/cli.h',
    'panglos/app/cli_cmd.h',

    'freertos/yield.h',
    'panglos/freertos/queue.h',
]

for path in files:
    make_link('../lib/panglos/src/', 'lib/panglos/src/', path)

# CLI stub files

files = [
    'cli_mutex.h',
    'cli_debug.h',
]

for path in files:
    make_link('../lib/', 'lib/', path)

#
#   Put the google test simulation in its own lib

#files = [
#    'gtest/gtest.h',
#    'gtest.cpp',
#]

#for path in files:
#    make_link('../panglos/', 'lib/gtest/src/', path)

# Create links to the other shared libraries
for lib in [ 'cli', 'printf' ]:
    make_link('../lib/', 'lib/', lib)

print("built library links ...")

# FIN
