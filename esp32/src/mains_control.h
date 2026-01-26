
#pragma once

#include "panglos/ring_buffer.h"

#include "temperature.h"

void mains_control_init(const char *topic);

    /*
     *
     */

namespace panglos {
    class LedStrip;
    class UART;
    class GPIO;
}

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

class PowerManager
{
public:
    struct Message
    {
        enum Type {
            M_MQTT,
            M_KEY,
            M_MODE,
        };

        enum Type type;
        int value;

        const char *text()
        { 
            static const LUT _lut[] = {
                { "MQTT", M_MQTT },
                { "KEY", M_KEY },
                { 0 },
            };
            return lut(_lut, type);
        }
    };

    typedef panglos::RingBuffer<struct Message> Messages;

    Messages messages;

    class Config
    {
    public:
        PowerControl *pc;
        panglos::LedStrip *leds;
        panglos::UART *uart;
        panglos::GPIO *button;
        int base;
        TemperatureControlConfig *temp_control;
    };

    PowerManager(int n) : messages(n) { }
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
    virtual int get_power() = 0;
    virtual int get_phase() = 0;
    virtual int get_temperature() = 0;
    virtual const char *get_error_mode() = 0;

    virtual void set_simulation(bool on, int power=0) = 0;
    virtual void set_pulse(int p) = 0;
    virtual void sim_phase(bool on, int phase) = 0;
    virtual void sim_temperature(bool on, int t) = 0;
    virtual void forward(const char *s) = 0;
    
    static PowerManager *create(const Config *config);
};

//  FIN
