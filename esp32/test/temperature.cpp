
#include <gtest/gtest.h>

#include "panglos/debug.h"
#include "panglos/debug.h"

#include "temperature.h"

// dummy devices
#include "devices.h"

using namespace panglos;

class t_SysSetup
{
public:
    mc_GPIO fan;
    mc_Temp temperature;

    t_SysSetup()
    {
        // make sure it looks a bit like a recent boot
        Time::set(1);
    }
    ~t_SysSetup()
    {
    }
};

    /*
     *
     */

TEST(Temperature, Update)
{
    t_SysSetup sys;
    TemperatureControlConfig config = {
        .fan = & sys.fan,
        .sensor = & sys.temperature,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    TemperatureControl tc(& config);

    for (int t = -20; t < 80; t++)
    {
        sys.temperature.t = t;
        tc.update();
        int temp = 0;
        tc.get_temperature(& temp);
        EXPECT_EQ(temp, t);
    }
}

TEST(Temperature, Alarm)
{
    t_SysSetup sys;
    TemperatureControlConfig config = {
        .fan = & sys.fan,
        .sensor = & sys.temperature,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    TemperatureControl tc(& config);

    for (int t = -20; t < 80; t++)
    {
        sys.temperature.t = t;
        tc.update();
        if (t < 40)
        {
            EXPECT_FALSE(tc.alarm());
        }
        else
        {
            EXPECT_TRUE(tc.alarm());
        }
    }
}

TEST(Temperature, Fan)
{
    t_SysSetup sys;
    TemperatureControlConfig config = {
        .fan = & sys.fan,
        .sensor = & sys.temperature,
        .fan_on = 28,
        .fan_off = 25,
        .alarm = 40,
    };

    TemperatureControl tc(& config);

    for (int t = -20; t < 80; t++)
    {
        sys.temperature.t = t;
        tc.update();
        if (t <= 28)
        {
            EXPECT_FALSE(sys.fan.state);
        }
        else
        {
            EXPECT_TRUE(sys.fan.state);
        }
    }

    for (int t = 80; t > -20; t--)
    {
        sys.temperature.t = t;
        tc.update();
        if (t >= 25)
        {
            EXPECT_TRUE(sys.fan.state);
        }
        else
        {
            EXPECT_FALSE(sys.fan.state);
        }
    }
}

//  FIN
