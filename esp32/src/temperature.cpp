

#include "panglos/debug.h"
#include "panglos/drivers/temperature.h"
#include "panglos/drivers/gpio.h"

#include "temperature.h"

TemperatureControl::TemperatureControl(TemperatureControlConfig *c)
:   config(*c),
    temperature(85),
    fan_state(false)
{
    if (config.fan)
    {
        // if we have no temperature sensor, turn the fan on all the time
        config.fan->set(config.sensor ? false : true);
    }
}

void TemperatureControl::check_temperature()
{
    panglos::TemperatureSensor *sensor = config.sensor;
    if (!sensor) return;
    if (!sensor->ready()) return;

    double t;
    if (sensor->get_temp(& t))
    {
        // TODO : control the fan to regulate the temperature
        temperature = (int) t;
        fan_control();
        //PO_DEBUG("%f", t);
    }
    sensor->start_conversion();
}

void TemperatureControl::set_fan(bool on)
{
    if (fan_state == on) return;
    fan_state = on;
    PO_DEBUG("fan %s t=%d", on ? "on" : "off", temperature);
    config.fan->set(on);
}

void TemperatureControl::fan_control()
{
    if (!config.fan) return;
    if (temperature < config.fan_off)
    {
        set_fan(false);
    }
    if (temperature > config.fan_on)
    {
        set_fan(true);
    }
}

void TemperatureControl::update()
{
    check_temperature();
}

bool TemperatureControl::alarm()
{
    return temperature >= config.alarm;
}

int TemperatureControl::get_temperature()
{
    return temperature;
}

//  FIN
