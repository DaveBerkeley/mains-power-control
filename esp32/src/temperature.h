
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
    bool fan_xor;
};

    /*
     *
     */

class TemperatureControl
{
    TemperatureControlConfig config;
    int temperature;
    bool fan_state;
    bool valid;

    void check_temperature();
    void set_fan(bool on);
    void fan_control();
public:

    TemperatureControl(TemperatureControlConfig *c);

    void update();
    bool alarm();
    bool get_temperature(int *t);

    void set_sensor(panglos::TemperatureSensor *);
    const TemperatureControlConfig *get_config() { return & config; }
};

//  FIN
