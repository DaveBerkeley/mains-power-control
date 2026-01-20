
#pragma once

#include "panglos/app/event.h"

    /*
     *
     */

class PowerControl
{
public:
    virtual ~PowerControl() { }
    virtual void update(int power) = 0;
    virtual int get_power() = 0;
    virtual int get_percent() = 0;

    static PowerControl *create(int load, int target);
};

    /*
     *
     */

void mains_control_init(const char *topic);

namespace panglos {
    class LedStrip;
    class UART;
    class GPIO;
}

class PowerManager
{
public:
    virtual ~PowerManager() { }

    typedef enum Mode
    {
        M_NONE = 0,
        M_OFF,  // Always OFF
        M_ON,   // Always ON
        M_ECO,  // Active Eco mode
        M_BASE, // Eco mode with minimum power floor
    }   Mode;

    static const LUT mode_lut[];

    static const char *mode_name(Mode m)
    {
        return lut(mode_lut, m);
    }

    virtual void set_mode(Mode m) = 0;    
    virtual Mode get_mode() = 0;    
    virtual void on_power(int _power) = 0;
    virtual void on_tick() = 0;
    virtual void on_idle() = 0;

    virtual int get_percent() = 0;

    static PowerManager *create(PowerControl *pc, panglos::LedStrip *_leds, panglos::UART *_uart, panglos::GPIO *gpio, int base);
};

//  FIN
