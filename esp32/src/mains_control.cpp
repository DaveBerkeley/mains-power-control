

#if defined(MAINS_POWER_CONTROL)

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(ESP32)
#include "hal/gpio_types.h"
#endif

#include "panglos/debug.h"

#include "cli/src/cli.h"

#include "panglos/network.h"

#include "panglos/json.h"
#include "panglos/mqtt.h"
#include "panglos/storage.h"
#include "panglos/object.h"
#include "panglos/network.h"
#include "panglos/time.h"
#include "panglos/verbose.h"

#include "panglos/drivers/timer.h"
#include "panglos/drivers/one_wire.h"
#include "panglos/drivers/temperature.h"
#include "panglos/drivers/ds18b20.h"
#include "panglos/drivers/led_strip.h"
#include "panglos/drivers/gpio.h"

#include "panglos/app/event.h"
#include "panglos/app/devices.h"
#include "panglos/app/cli_server.h"

#include "mains_control.h"

using namespace panglos;

static VERBOSE(app, "app", false);

    /*
     *
     */

#if defined(ESP32)

#include "panglos/esp32/gpio.h"
#include "panglos/esp32/rmt_strip.h"
#include "panglos/esp32/timer.h"
#include "panglos/drivers/one_wire.h"
#include "panglos/drivers/ds18b20.h"

struct LedsDef {
    uint32_t pin;
    int n;
    RmtLedStrip::Type type;
};

static bool leds_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct LedsDef *def= (struct LedsDef*) arg;

    RmtLedStrip *leds = RmtLedStrip::create(def->n, 24, def->type);
    ASSERT(leds);
    const bool ok = leds->init(0, def->pin);
    leds->set_all(0x10, 0x10, 0x10);
    leds->send();
    dev->add(Objects::objects, leds);
    return ok;
}

static const GPIO_DEF button_def = { GPIO_NUM_8, ESP_GPIO::IP | ESP_GPIO::PU, false };
static const GPIO_DEF fan_def = { GPIO_NUM_6, ESP_GPIO::OP, false };

#define UART_BAUD 115200
#define UART_TX GPIO_NUM_5
#define UART_RX GPIO_NUM_4

static const struct UART_DEF uart_def = { .chan=0, .rx=UART_RX, .tx=UART_TX, .baud=UART_BAUD, };

static const struct LedsDef leds_def = { .pin=GPIO_NUM_9, .n=2, .type=RmtLedStrip::Type::WS2812B };

static struct DefOneWire ow_def = { .pin = GPIO_NUM_7 };

static const char *needs_onewire[] = { "onewire", 0 };

static Device _board_devs[] = {
    Device("button", 0, gpio_init, (void*) & button_def),
    Device("fan",  0, gpio_init, (void*) & fan_def),
    Device("uart", 0, uart_init, (void*) & uart_def),
    Device("leds", 0, leds_init, (void*) & leds_def),
    // have to use bitbang interface as the RMT one can't coexist with the LED driver
    Device("onewire", 0, init_onewire_bitbang, & ow_def, Device::F_CAN_FAIL),
    Device("temperature", needs_onewire, init_ds18b20, (void*) "onewire", Device::F_CAN_FAIL),
    Device(0, 0, 0, 0, 0),
};

Device *board_devs = _board_devs;

void board_init()
{
    PO_DEBUG("");
    EventHandler::add_handler(Event::INIT, net_cli_init, 0);

    const char *topic = 0;

    {
        Storage db("mqtt");
        char stopic[48];
        size_t s = sizeof(stopic);
        if (db.get("topic", stopic, & s))
        {
            topic = strdup(stopic);
        }
    }

    mains_control_init(topic);
}

#endif // ESP32

    /*
     *
     */

#include "lut.h"

    /*
     *
     */

const LUT PowerManager::mode_lut[] = {
    { "off", M_OFF },
    { "on", M_ON },
    { "eco", M_ECO },
    { "base", M_BASE },
    { 0 },
};

class _PowerManager : public PowerManager
{
    PowerControl *pc;
    LedStrip *leds;
    FmtOut stm32;
    GPIO *button;
    TemperatureControl temp_control;
    int ticks;
    int power; // +- import/export power from MQTT
    int pulse;
    int phase;
    bool flash;
    bool sw_state; // previous state of switch GPIO
    int percent;
    const int watchdog_period = 100 * 10; // 
    const int watchdog_reboot = 100 * 60; // 
    Time::tick_t last_mqtt;

    Mode mode;
    int base; // base percent

    bool simulation;
    int sim_power;
    int phase_sim;
    int phase_sim_value;

public:

    virtual int get_phase() override
    {
        return phase;
    }

    virtual int get_power() override
    {
        return power;
    }

    virtual void set_simulation(bool on, int power=0) override
    {
        PO_DEBUG("simulation=%d power=%d", on, power);
        simulation = on;
        sim_power = power;
    }

    virtual void set_pulse(int p) override
    {
        PO_DEBUG("pulse=%d", p);
        pulse = p;
    }

    virtual void sim_phase(bool on, int phase) override
    {
        phase_sim = on;
        phase_sim_value = phase;
    }

    virtual Mode get_mode() override
    {
        return mode;
    }    

    virtual int get_percent() override
    {
        return percent;
    }    

    virtual void set_mode(Mode m) override
    {
        PO_DEBUG("mode=%s", mode_name(m));

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        uint8_t bright = 0x40;

        switch (m)
        {
            case M_NONE : break;
            case M_OFF  : break;
            case M_ON   : r = bright; break;
            case M_ECO  : g = bright; break;
            case M_BASE : b = bright; break;
            default     : 
            {
                PO_WARNING("unknown mode %d", m);
                return;
            }
        }

        mode = m;
        leds->set(0, g, r, b);
        leds->send();
    }

    int calc_percent()
    {
        switch (mode)
        {
            case M_OFF : return 0;
            case M_ON : return 99;
            case M_NONE : // fall thru
            case M_ECO :
            case M_BASE :
            default : break;
        }

        ASSERT(pc);
        int p = pc->get_percent();

        if (mode == M_BASE)
            if (p < base)
                p = base;

        return p;
    }

    static int percent_to_triac(int ph)
    {
        return power_lut[ph];
    }

    void set_power_indicator(int power)
    {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (flash)
        {
            int bright = abs(power) / 10;
            if (bright > 255)
                bright = 255;
            if (power > 0)
                r = uint8_t(bright); // import
            else
                g = uint8_t(bright); // export
        }
        flash = !flash;

        leds->set(1, g, r, b);
        leds->send();
    }

    enum Error { E_NONE, E_WIFI, E_MQTT, E_TEMP };

    void set_error_indicator(enum Error err)
    {
        flash = !flash;
        const uint8_t bright = 20;
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        switch (err)
        {
            case E_WIFI : { b = bright; g = bright; break; }
            case E_MQTT : { b = bright; break; }
            case E_TEMP : { r = bright; break; }
            case E_NONE : break; // fall thru
            default : return;
        }

        leds->set(flash, g, r, b);
        leds->set(!flash, 0, 0, 0);
        leds->send();
    }

    bool wifi_error()
    {
        // return true if wifi is not connected
        static Network *net = 0;
        if (!net)
        {
            net = (Network *)Objects::objects->get("net");
        }

        if (!net) return true;

        Interface *iface = net->get_interface();
        if (!iface) return true;        
        return !iface->is_connected();
    }

public:
    _PowerManager(const Config *config)
    :   PowerManager(32),
        pc(config->pc),
        leds(config->leds),
        stm32(config->uart),
        button(config->button),
        temp_control(config->temp_control),
        ticks(0),
        power(0),
        pulse(100),
        flash(false),
        sw_state(false),
        percent(0),
        last_mqtt(0),
        mode(M_NONE),
        base(config->base),
        simulation(false),
        sim_power(0),
        phase_sim(false),
        phase_sim_value(0)
    {
        ASSERT(config);
        ASSERT(config->pc);
        ASSERT(config->leds);
        ASSERT(config->uart);
    }

    virtual void on_power(int _power) override
    {
        if (simulation) return; // ignore MQTT data
        struct Message msg = { 
            .type = Message::M_MQTT,
            .value = _power,
        };
        messages.push(msg);
    }

    virtual void on_tick() override
    {
        ticks += 1;

        if (simulation && ((ticks % 100) == 0)) {

            struct Message msg = { 
                .type = Message::M_MQTT,
                .value = sim_power,
            };
            messages.push(msg);
        }
        
        // called in the esp_timer thread
        // timer tick @ 100Hz
        bool state = button->get();
        if (state == sw_state) return;
        sw_state = state;
        if (state) return; // ignore key releases

        struct Message msg = { .type = Message::M_KEY };
        messages.push(msg);
    }

    void set_phase(int p)
    {
        if (app.verbose) PO_DEBUG("phase=%d percent=%d power=%d t=%d", p, percent, power, get_temperature());
        stm32.printf("phase %d %d # %d\r\n", p, pulse, ticks);
        phase = p;
        stm32.printf("led %d\r\n", flash);
    }

    void handle_power(int _power, bool update_leds=true)
    {
        // event trigger by MQTT data, runs in main thread.
        power = _power;
        ASSERT(pc);
        pc->update(power);
        percent = calc_percent();
        const int triac = percent_to_triac(percent);
        //PO_DEBUG("power=%d percent=%d triac=%d mode=%s", power, percent, triac, mode_name(mode));

        set_phase(phase_sim ? phase_sim_value : triac);

        if (update_leds)
        {
            set_power_indicator(power);
        }
    }

    virtual void forward(const char *s) override
    {
        stm32.printf("%s", s);
    }

    void handle_key()
    {
        // advance the mode
        switch (mode)
        {
            case M_NONE : return;
            case M_OFF  : set_mode(M_ON);   break;
            case M_ON   : set_mode(M_ECO);  break;
            case M_ECO  : set_mode(M_BASE); break;
            case M_BASE : set_mode(M_OFF);  break;
            default     : ASSERT(0);
        }
    }

    virtual int get_temperature() override
    {
        return temp_control.get_temperature();
    }

    virtual void on_idle() override
    {
        // polled in the main thread

        if (mode == M_NONE)
        {
            // start in ECO mode by default
            set_mode(M_ECO);
        }

        Time::tick_t now = Time::get();

        if (Time::elapsed(last_mqtt, watchdog_reboot))
        {
            PO_DEBUG("TIMEOUT!");
            ASSERT(0);
        }

        if ((now % 100) == 0)
        {
            temp_control.update();
        }

        // TODO : have a defined error state
        static bool error = false;
 
        // check for error conditions every 1/4 s
        if ((now % 25) == 0)
        {
            error = false;
            
            if (temp_control.alarm())
            {
                set_error_indicator(E_TEMP);
                error = true;
            }
            else if (wifi_error())
            {
                set_error_indicator(E_WIFI);
                error = true;
            }
            else if (Time::elapsed(last_mqtt, watchdog_period))
            {
                set_error_indicator(E_MQTT);
                error = true;
            }

            if (error && ((now % 100) == 0))
            {
                // tell the system we have zero power available as mqtt not working
                handle_power(10000, false);
            }
        }

        if (!messages.size()) return;

        struct Message msg = messages.pop();

        switch (msg.type)
        {
            case Message::M_KEY  :
            { 
                handle_key();
                break;
            }
            case Message::M_MQTT :
            {
                last_mqtt = Time::get();
                if (!error) handle_power(msg.value);
                break;
            }
            case Message::M_MODE :
            {
                set_mode(Mode(msg.value));
                break;
            }
            default : PO_ERROR("bad msg.type '%s'", msg.text());
        }
    }
};

    /*
     *
     */

PowerManager *PowerManager::create(const PowerManager::Config *config)
{
    ASSERT(config);
    return new _PowerManager(config);
}

    /*
     *  Parse the JSON in the MQTT response
     */

// TODO : add some of these standard parser functions to the lib.
static void on_control(void *arg, panglos::json::Section *sec, panglos::json::Match::Type, const char **)
{
    ASSERT(arg);
    int *b = (int*) arg;
    int value = 0;
    sec->to_int(& value);
    *b = value;
}

static void on_json(void *arg, const char *data, int len)
{
    json::Section sec(data, & data[len-1]);
    json::Match match;

    const char *keys[] = { "W", 0 };

    int power = -1;

    json::Match::Item p1 = {
        .keys = keys,
        .on_match = on_control,
        .arg = & power,
    };

    match.add_item(& p1);

    json::Parser parser(& match);
    parser.parse(& sec);

    ASSERT(arg);
    _PowerManager *pm = (_PowerManager *) arg;
    pm->on_power(power);
}

    /*
     *
     */

static void on_timer(Timer *, void *arg)
{
    ASSERT(arg);
    _PowerManager *pm = (_PowerManager *) arg;
    pm->on_tick();
}

static bool on_idle(void *arg, Event *, Event::Queue *)
{
    ASSERT(arg);
    _PowerManager *pm = (_PowerManager *) arg;
    pm->on_idle();
    return false;
}

    /*
     *
     */

class _PowerControl : public PowerControl
{
    int power;
    double percent;
    int load;
    int target;
public:
    _PowerControl(int _load=1000, int _target=-40)
    :   power(0),
        percent(0),
        load(_load),
        target(_target)
    {
    }

    virtual void update(int net) override
    {
        power = net; // net metered power

        // we want to push net to approx zero
        // so adjust burn to tend towards net == target.
        // bias the target to a slight export. 
        // This also gives us a noise margin
        // to prevent the system switching off with slight import excursions.
        const int error = net - target; // Watts

        const double ratio_up = 0.25;  // ramp up slowly
        const double ratio_down = 0.5; // ramp down quickly

        const double available = -(100.0 * error) / load; // percent
        double delta = 0; // percent
 
        if ((error > 0) && (error > abs(target)))
        {
            // importing
            delta = ratio_down * available;
        }
        else
        {
            // exporting
            delta = ratio_up * available;
        }

        percent += delta;
        percent = clamp_percent(percent);
    }

    static double clamp_percent(double x)
    {
        if (x < 0) return 0;
        if (x >= 99) return 99;
        return x;
    }

    virtual int get_power() override
    {
        return power;
    }

    virtual int get_percent() override
    {
        return int(percent);
    }
};

PowerControl *PowerControl::create(int load, int target)
{
    return new _PowerControl(load, target);
}

    /*
     *  Install the application specific CLI commands
     */

extern CliCommand cli_cmds[];

static bool mc_cli_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    ASSERT(arg);
    PowerManager *pm = (PowerManager*) arg;
    
    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);

    for (CliCommand *cmd = cli_cmds; cmd->cmd; cmd++)
    {
        cmd->ctx = pm;
        cli_append(cli, cmd);
    }

    return false; // INIT handlers must return false so multiple handlers can be run
}

    /*
     *
     */

static void init_mqtt(const char *topic, PowerManager *pm)
{
    MqttClient *mqtt = (MqttClient *) Objects::objects->get("mqtt");
    if (!mqtt)
    {
        PO_ERROR("No MQTT service found");
        return;
    }
    if (!topic)
    {
        PO_ERROR("No MQTT topic found");
        return;
    }

    static MqttSub sub = {
        .topic = topic,
        .handler = on_json,
        .arg = pm,
    };

    mqtt->add(& sub);
}

    /*
     *
     */

void mains_control_init(const char *topic)
{
    PO_DEBUG("");
    LedStrip *leds = (LedStrip*) Objects::objects->get("leds");
    ASSERT(leds);
    UART *uart = (UART*) Objects::objects->get("uart");
    ASSERT(uart);
    GPIO *button = (GPIO*) Objects::objects->get("button");
    ASSERT(button);

    // temperature control
    GPIO *fan = (GPIO*) Objects::objects->get("fan");
    TemperatureSensor *temperature = (TemperatureSensor*) Objects::objects->get("temperature");
 
    Storage db("app");
    int32_t base = 20;
    int32_t load = 1000;
    int32_t target = -40;
    int32_t fan_on = 26;
    int32_t fan_off = 25;
    int32_t alarm = 40;
    db.get("base", & base);
    db.get("load", & load);
    db.get("target", & target);
    db.get("fan_on", & target);
    db.get("fan_off", & target);
    db.get("alarm", & target);
    PO_DEBUG("base=%d load=%d target=%d", (int) base, (int) load, (int) target);
 
    PowerControl *pc = PowerControl::create(load, target);

    TemperatureControlConfig tc_config = {
        .fan = fan,
        .sensor = temperature,
        .fan_on = fan_on,
        .fan_off = fan_off,
        .alarm = alarm,
    };

    PowerManager::Config config = {
        .pc = pc,
        .leds = leds,
        .uart = uart,
        .button = button,
        .base = base,
        .temp_control = & tc_config,
    };

    PowerManager *pm = PowerManager::create(& config);
    Objects::objects->add("power_manager", pm);

    // Timer used to read the UI GPIO
    Timer *timer = Timer::create();
    ASSERT(timer);
    timer->set_handler(on_timer, pm);
    timer->set_period(10000); // in us
    timer->start(true);

    init_mqtt(topic, pm);

    // IDLE events as are used as the main loop
    EventHandler::add_handler(Event::IDLE, on_idle, pm);

    // add new cli commands once the CLI is running
    EventHandler::add_handler(Event::INIT, mc_cli_init, pm);    
}

#endif  //  MAINS_POWER_CONTROL

//  FIN
