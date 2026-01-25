
#pragma once

    /*
     *
     */

namespace panglos {
    class GPIO;
    class TemperatureSensor;
}

    /*
     *
     */

struct TemperatureControlConfig
{
    panglos::GPIO *fan;
    panglos::TemperatureSensor *sensor;
    int fan_on;
    int fan_off;
    int alarm;
};

    /*
     *
     */

class TemperatureControl
{
public:
    TemperatureControlConfig config;
    int temperature;
    bool fan_state;

    void check_temperature();
    void set_fan(bool on);
    void fan_control();

    TemperatureControl(TemperatureControlConfig *c);

    void update();
    bool alarm();
    int get_temperature();
};

//  FIN
