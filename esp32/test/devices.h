
#pragma once

#include <list>

    /*
     *
     */

#include "panglos/time.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/uart.h"
#include "panglos/drivers/led_strip.h"
#include "panglos/drivers/temperature.h"


class mc_GPIO : public panglos::GPIO
{
    virtual void set(bool on) override
    {
        state = on;
        PO_DEBUG("state=%d", state);
    }
    virtual bool get() override
    {
        return state;
    }
    virtual void toggle() override
    {
        set(!get());
    }
public:
    bool state;

    mc_GPIO() : state(true) { }

};

class mc_LedStrip : public panglos::LedStrip
{
public:
    struct LedState {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    struct LedState state[2];

    mc_LedStrip()
    {
        memset(state, 0, sizeof(state));
    }

    virtual void set(int led, uint8_t r, uint8_t g, uint8_t b) override
    {
        //PO_DEBUG("idx=%d r=%d g=%d b=%d", led, r, g, b);
        EXPECT_TRUE(led >= 0);
        EXPECT_TRUE(led < num_leds());
        state[led].r = g; // r-g swapped on YF923 devices
        state[led].g = r;
        state[led].b = b;
    }
    virtual void set_all(uint8_t r, uint8_t g, uint8_t b) override
    {
        PO_DEBUG("r=%d g=%d b=%d", r, g, b);
        for (int i = 0; i < num_leds(); i++)
        {
            state[i].r = g; // r-g swapped on YF923 devices
            state[i].g = r;
            state[i].b = b;
        }
    }
    virtual bool send() override
    {
        //PO_DEBUG("");
        return true;
    }
    virtual int num_leds() override
    {
        //PO_DEBUG("");
        return 2;
    }    
};

class mc_UART : public panglos::UART
{
public:
    char buff[64];
    size_t idx;
    typedef std::list<std::string> List;

    List strings;

    mc_UART() : idx(0) { }

    virtual int rx(char* data, int n) override
    {
        PO_DEBUG("%p %d", data, n);
        return n;
    }
    virtual int tx(const char* data, int n) override
    {
        //PO_DEBUG("%p '%c' %d", data, *data, n);
        for (int i = 0; i < n; i++)
        {
            ASSERT(idx < sizeof(buff));
            switch (data[i])
            {
                case '\r' : continue;
                case '\n':
                {
                    buff[idx] = '\0';
                    strings.push_back(buff);
                    //PO_DEBUG("'%s'", buff);
                    idx = 0;
                    continue;
                }
                default : break;
            }
            buff[idx++] = data[i];
        }
        return n;
    }

    bool pop_string(char *buff, size_t len)
    {
        if (strings.size() == 0) return false;
        std::string s = strings.front();
        strncpy(buff, s.c_str(), len);
        strings.pop_front();
        return true;
    }
};

class mc_Temp : public panglos::TemperatureSensor
{
    virtual bool get_temp(double *pt) override
    {
        if (pt) *pt = t;
        return true;
    }
    virtual bool ready() override
    {
        return true;
    }
    virtual bool start_conversion() override
    {
        return true;
    }

public:
    double t;

    mc_Temp() : t(0) { }
};

//  FIN
