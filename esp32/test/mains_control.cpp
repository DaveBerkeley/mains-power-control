
#include <list>

#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/object.h"
#include "panglos/time.h"

#include "panglos/drivers/gpio.h"
#include "panglos/drivers/uart.h"
#include "panglos/drivers/led_strip.h"
#include "panglos/drivers/temperature.h"

#include "panglos/app/event.h"

#include "mains_control.h"

using namespace panglos;

    /*
     *
     */

class mc_GPIO : public panglos::GPIO
{
public:
    bool state;

    mc_GPIO() : state(true) { }

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
};

class mc_LedStrip : public LedStrip
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
public:
    double t;

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

    mc_Temp() : t(0) { }
};

     /*
     *  Simulate a system populated with Devices
     */

class SysSetup
{
public:
    mc_GPIO button;
    mc_GPIO fan;
    mc_LedStrip leds;
    mc_UART uart;
    mc_Temp temperature;

    SysSetup()
    {
        // make sure it looks a bit like a recent boot
        Time::set(1);

        if (!Objects::objects)
        {
            Objects::objects = Objects::create();
        }

        Objects::objects->add("button", & button);
        Objects::objects->add("fan", & fan);
        Objects::objects->add("leds", & leds);
        Objects::objects->add("uart", & uart);
    }
    ~SysSetup()
    {
        delete Objects::objects;
        Objects::objects = 0;
    }
};

    /*
     *
     */

TEST(MainsControl, Modes)
{
    SysSetup sys;
    PowerControl *control = PowerControl::create(1000, -30);
    TemperatureControlConfig tc_config = {
        .fan = 0,
        .sensor = 0,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    const PowerManager::Config config = {
        .pc = control,
        .leds = & sys.leds,
        .uart = & sys.uart,
        .button = & sys.button,
        .base = 20,
        .temp_control = & tc_config,
    };
    PowerManager *pm = PowerManager::create(& config);
    const uint8_t bright = 0x40;

    mc_LedStrip::LedState & state = sys.leds.state[0];

    // check modes

    // starts in ECO
    pm->on_idle();
    EXPECT_STREQ(pm->mode_name(pm->get_mode()), "eco");
    EXPECT_EQ(state.r, 0);
    EXPECT_EQ(state.g, bright);
    EXPECT_EQ(state.b, 0);

    pm->set_mode(PowerManager::M_OFF);
    pm->on_idle();
    EXPECT_STREQ(pm->mode_name(pm->get_mode()), "off");
    EXPECT_EQ(state.r, 0);
    EXPECT_EQ(state.g, 0);
    EXPECT_EQ(state.b, 0);

    pm->set_mode(PowerManager::M_ON);
    pm->on_idle();
    EXPECT_STREQ(pm->mode_name(pm->get_mode()), "on");
    EXPECT_EQ(state.r, bright);
    EXPECT_EQ(state.g, 0);
    EXPECT_EQ(state.b, 0);

    pm->set_mode(PowerManager::M_ECO);
    pm->on_idle();
    EXPECT_STREQ(pm->mode_name(pm->get_mode()), "eco");
    EXPECT_EQ(state.r, 0);
    EXPECT_EQ(state.g, bright);
    EXPECT_EQ(state.b, 0);

    pm->set_mode(PowerManager::M_BASE);
    pm->on_idle();
    EXPECT_STREQ(pm->mode_name(pm->get_mode()), "base");
    EXPECT_EQ(state.r, 0);
    EXPECT_EQ(state.g, 0);
    EXPECT_EQ(state.b, bright);

    delete pm;
    delete control;
}

TEST(MainsControl, Keys)
{
    SysSetup sys;
    PowerControl *control = PowerControl::create(1000, -30);
    TemperatureControlConfig tc_config = {
        .fan = 0,
        .sensor = 0,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    const PowerManager::Config config = {
        .pc = control,
        .leds = & sys.leds,
        .uart = & sys.uart,
        .button = & sys.button,
        .base = 20,
        .temp_control = & tc_config,
    };
    PowerManager *pm = PowerManager::create(& config);

    // check that the key presses cycle through the modes

    struct State
    {
        bool key;
        PowerManager::Mode mode;
    };

    static const struct State states[] = {
        {   true, PowerManager::M_ECO },
        {   true, PowerManager::M_ECO },
        {   false, PowerManager::M_BASE },
        {   true, PowerManager::M_BASE },
        {   true, PowerManager::M_BASE },
        {   true, PowerManager::M_BASE },
        {   false, PowerManager::M_OFF },
        {   false, PowerManager::M_OFF },
        {   false, PowerManager::M_OFF },
        {   false, PowerManager::M_OFF },
        {   true, PowerManager::M_OFF },
        {   true, PowerManager::M_OFF },
        {   false, PowerManager::M_ON },
        {   false, PowerManager::M_ON },
        {   false, PowerManager::M_ON },
        {   true, PowerManager::M_ON },
        {   true, PowerManager::M_ON },
        {   false, PowerManager::M_ECO },
        {   false, PowerManager::M_ECO },
        {   false, PowerManager::M_ECO },
        {   true, PowerManager::M_ECO },
    };

    //starts in ECO
    pm->on_idle();
    EXPECT_EQ(pm->get_mode(), PowerManager::M_ECO);

    for (const struct State &state : states)
    {
        sys.button.state = state.key;
        pm->on_tick();
        pm->on_idle();
        EXPECT_EQ(pm->get_mode(), state.mode);
    }

    delete pm;
    delete control;
}

TEST(MainsControl, Power)
{
    SysSetup sys;
    PowerControl *control = PowerControl::create(1000, -30);
    TemperatureControlConfig tc_config = {
        .fan = 0,
        .sensor = 0,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    const PowerManager::Config config = {
        .pc = control,
        .leds = & sys.leds,
        .uart = & sys.uart,
        .button = & sys.button,
        .base = 20,
        .temp_control = & tc_config,
    };
    PowerManager *pm = PowerManager::create(& config);

    // always full on
    pm->set_mode(PowerManager::M_ON);
    for (int p = -2000; p < 2000; p += 10)
    {
        pm->on_power(p);
        pm->on_idle();
        EXPECT_EQ(pm->get_percent(), 99);
    }

    // always off
    pm->set_mode(PowerManager::M_OFF);
    for (int p = -2000; p < 2000; p += 10)
    {
        pm->on_power(p);
        pm->on_idle();
        EXPECT_EQ(pm->get_percent(), 0);
    }

    // TODO : test for active modes

    delete pm;
    delete control;
}

    /*
     * P_net(t) = P_base_load(t) + α(t)·P_max - P_solar(t)
     *
     * P_base_load: uncontrollable loads (kettle, fridge, etc.) - disturbances
     * α(t): your control input, ∈ [0,1]
     * P_max: maximum load power (unknown parameter)
     * P_solar: disturbance (weather dependent)
     *
     */

#if 0

P_net(t) = P_base_load(t) + α(t)·P_max - P_solar(t)

#endif

class PowerSim
{
public:
    virtual ~PowerSim() { }

    virtual bool run() = 0;
    virtual int get_power() = 0;
    virtual int get_time() = 0;
};

    /*
     *
     */

struct Line
{
    int secs;
    int watts;
};

static void parse_line(char *line, struct Line *info)
{
    // "hh:mm:ss power"
    char *b = line;
    char *save = 0;
    const char *hh = strtok_r(b, ":", & save);
    const char *mm = strtok_r(0, ":", & save);
    const char *ss = strtok_r(0, " ", & save);
    const char *pp = strtok_r(0, "\n", & save);

    char *end = 0;
    int secs = 0;
    secs += (int) strtol(hh, & end, 10);
    secs *= 60;
    secs += (int) strtol(mm, & end, 10);
    secs *= 60;
    secs += (int) strtol(ss, & end, 10);
    int watts = (int) strtol(pp, & end, 10);

    info->secs = secs;
    info->watts = watts;
}

class RealData : public PowerSim
{
    int tick;
    FILE *file;
    int net;
public:
    RealData(const char *path)
    :   tick(0),
        file(0),
        net(0)
    {
        file = fopen(path, "r");
        ASSERT_ERROR(file, "path=%s", path);
    }
    ~RealData()
    {
        fclose(file);
    }

    virtual int get_time() override
    {
        return tick;
    }

    bool read()
    {
        tick += 1;

        char buff[128];
        char *s = fgets(buff, sizeof(buff), file);
        if (!s) return false;
        //PO_DEBUG("'%s'", s);

        struct Line info;
        parse_line(buff, & info);
        //PO_DEBUG("secs=%d net=%d", info.secs, info.solar);
        net = info.watts;
        
        return true;
    }

    virtual bool run() override
    {
        //return read();
        const int lo = 47483;
        const int hi = 50291;
        while (tick < lo)
        {
            if (!read())
                return false;
        }
        if (tick > hi)
            return false;

        return read();
    }

    virtual int get_power() override
    {
        return net;
    }
};

    /*
     *
     */

TEST(MainsControl, Solar)
{
    const int load = 1500;
    PowerControl *control = PowerControl::create(load, -30);

    RealData sim("test/solar3.log");
    int burn = 0;

    int total_burn = 0;
    int total_import = 0;
    int total_meter = 0;

    while (sim.run())
    {
        const int meter = sim.get_power();
        const int net = meter + burn;
        control->update(net);
        const int percent = control->get_percent();
        burn = load * percent / 100;
        //printf("xsim %d %d %d %d %d\n", sim.get_time(), meter, net, percent, burn);

        total_burn += burn;
        if (meter > 0)
            total_meter += meter;
        if (net > 0)
            total_import += net;
    }

    const int Wh = 60 * 60;
    total_meter /= Wh;
    total_burn /= Wh;
    total_import /= Wh;

    fprintf(stderr, "meter=%d burn=%d net=%d\n",
            total_meter, total_burn, total_import);
    int excess = total_import - total_meter;
    fprintf(stderr, "extra_imports=%d extra_free=%d \n", 
            excess,
            total_burn - excess
            );

    delete control;
}

    /*
     *
     */

TEST(MainsControl, Fan)
{
    SysSetup sys;
    PowerControl *control = PowerControl::create(1000, -30);
    TemperatureControlConfig tc_config = {
        .fan = & sys.fan,
        .sensor = & sys.temperature,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    const PowerManager::Config config = {
        .pc = control,
        .leds = & sys.leds,
        .uart = & sys.uart,
        .button = & sys.button,
        .base = 20,
        .temp_control = & tc_config,
    };
    PowerManager *pm = PowerManager::create(& config);

    sys.temperature.t = 10;
    for (int i = 1; i < 1000; i++)
    {
        if (i > 300) sys.temperature.t = 30;
        if (i > 500) sys.temperature.t = 20;
        if (i > 800) sys.temperature.t = 30;
        Time::set(i);
        pm->on_power(100);
        pm->on_idle();
        // valid on 100ms
        if ((Time::get() % 100) != 0) continue;

        if (i <= 300) { EXPECT_EQ(sys.fan.state, false); }
        else if (i <= 500) { EXPECT_EQ(sys.fan.state, true); }
        else if (i <= 800) { EXPECT_EQ(sys.fan.state, false); }
        else { EXPECT_EQ(sys.fan.state, true); }
    }
 
    delete pm;
    delete control;
}

//  FIN
