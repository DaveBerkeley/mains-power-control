

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
#include "panglos/ring_buffer.h"
#include "panglos/network.h"
#include "panglos/time.h"
#include "panglos/verbose.h"

#include "panglos/drivers/timer.h"

#include "panglos/esp32/gpio.h"
#include "panglos/esp32/rmt_strip.h"
#include "panglos/esp32/timer.h"

#include "panglos/app/event.h"
#include "panglos/app/devices.h"
#include "panglos/app/cli_server.h"

//#include "targets/board.h"
#include "mains_control.h"

using namespace panglos;

static VERBOSE(app, "app", false);

    /*
     *
     */

#if defined(ESP32)

struct LedsDef {
    uint32_t pin;
    int n;
};

static bool leds_init(Device *dev, void *arg)
{
    PO_DEBUG("");
    ASSERT(arg);
    struct LedsDef *def= (struct LedsDef*) arg;

    RmtLedStrip *leds = RmtLedStrip::create(def->n);
    ASSERT(leds);
    const bool ok = leds->init(0, def->pin);
    leds->set_all(0x10, 0x10, 0x10);
    leds->send();
    dev->add(Objects::objects, leds);
    return ok;
}

static const GPIO_DEF sw_def = { GPIO_NUM_8, ESP_GPIO::IP | ESP_GPIO::PU, false };
static const GPIO_DEF sda_def = { GPIO_NUM_6, ESP_GPIO::OP, false };
static const GPIO_DEF scl_def = { GPIO_NUM_7, ESP_GPIO::OP, false };

#define UART_BAUD 115200
#define UART_TX GPIO_NUM_5
#define UART_RX GPIO_NUM_4

static const struct UART_DEF uart_def = { .chan=0, .rx=UART_RX, .tx=UART_TX, .baud=UART_BAUD, };

static const struct LedsDef leds_def = { .pin=GPIO_NUM_9, .n=2 };

static Device _board_devs[] = {
    Device("sw",   0, gpio_init, (void*) & sw_def),
    Device("sda",  0, gpio_init, (void*) & sda_def),
    Device("scl",  0, gpio_init, (void*) & scl_def),
    Device("uart", 0, uart_init, (void*) & uart_def),
    Device("leds", 0, leds_init, (void*) & leds_def),
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

static const int power_lut_50Hz[100] = {
    0,    8679, 8481, 8321, 8183,
    8063, 7954, 7851, 7759, 7670,
    7587, 7510, 7433, 7361, 7292,
    7224, 7160, 7095, 7034, 6974,
    6914, 6857, 6802, 6745, 6694,
    6639, 6588, 6536, 6484, 6433,
    6384, 6335, 6284, 6238, 6189,
    6141, 6095, 6046, 6000, 5954,
    5909, 5863, 5817, 5771, 5725,
    5682, 5636, 5591, 5545, 5502,
    5456, 5410, 5367, 5321, 5275,
    5230, 5184, 5138, 5092, 5046,
    5000, 4955, 4909, 4860, 4814,
    4766, 4717, 4668, 4619, 4568,
    4519, 4468, 4416, 4362, 4310,
    4256, 4201, 4144, 4087, 4026,
    3969, 3906, 3843, 3777, 3711,
    3640, 3568, 3494, 3413, 3330,
    3244, 3150, 3050, 2941, 2817,
    2683, 2522, 2325, 2044, 1001,
};

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
    GPIO *sw;
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

    int get_phase()
    {
        return phase;
    }

    int get_power()
    {
        return power;
    }

    void set_simulation(bool on, int power=0)
    {
        PO_DEBUG("simulation=%d power=%d", on, power);
        simulation = on;
        sim_power = power;
    }

    void set_pulse(int p)
    {
        PO_DEBUG("pulse=%d", p);
        pulse = p;
    }

    void sim_phase(bool on, int phase)
    {
        phase_sim = on;
        phase_sim_value = phase;
    }

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

    typedef RingBuffer<struct Message> Messages;

    Messages messages;

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
        return power_lut_50Hz[ph];
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

    enum Error { M_WIFI, M_MQTT };

    void set_error_indicator(enum Error err)
    {
        flash = !flash;
        const uint8_t bright = 20;
        uint8_t r = 0;
        uint8_t b = 0;

        switch (err)
        {
            case M_WIFI : { b = bright; break; }
            case M_MQTT : { r = bright; break; }
            default : return;
        }

        leds->set(flash, 0, r, b);
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
    _PowerManager(PowerControl *_pc, LedStrip *_leds, UART *_uart, GPIO *gpio, int _base)
    :   pc(_pc),
        leds(_leds),
        stm32(_uart),
        sw(gpio),
        ticks(0),
        power(0),
        pulse(100),
        flash(false),
        sw_state(false),
        percent(0),
        last_mqtt(0),
        mode(M_NONE),
        base(_base),
        simulation(false),
        sim_power(0),
        phase_sim(false),
        phase_sim_value(0),
        messages(32)
    {
        ASSERT(leds);
        ASSERT(_uart);
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
        bool state = sw->get();
        if (state == sw_state) return;
        sw_state = state;
        if (state) return; // ignore key releases

        struct Message msg = { .type = Message::M_KEY };
        messages.push(msg);
    }

    void set_phase(int p)
    {
        if (app.verbose) PO_DEBUG("phase=%d percent=%d power=%d", p, percent, power);
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

    void forward(const char *s)
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

        // check for error conditions every 1/4 s
        if ((now % 25) == 0)
        {
            bool error = false;
            
            if (wifi_error())
            {
                set_error_indicator(M_WIFI);
                error = true;
            }
            else if (Time::elapsed(last_mqtt, watchdog_period))
            {
                set_error_indicator(M_MQTT);
                error = true;
            }

            if (error && ((now % 100) == 0))
            {
                // tell the system we have zero power available as mqtt not working
                handle_power(0, false);
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
                handle_power(msg.value);
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

PowerManager *PowerManager::create(PowerControl *pc, LedStrip *leds, UART *uart, GPIO *gpio, int base)
{
    return new _PowerManager(pc, leds, uart, gpio, base);
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
        const int error = net - target;

        const double ratio_up = 0.25;
        //const double ratio_down = 0.25;

        const double available = clamp_percent(-(100 * error) / load);
        double delta = 0;
 
        if ((error > 0) && (error > abs(target)))
        {
            // importing
            //delta = ratio_down * available;
            delta = 0;
        }
        else
        {
            // exporting
            delta = ratio_up * available;
        }

        if (error > (abs(target) / 2))
        {
            // force downward change if we are importing
            //if (delta > -1)
            //    delta = -1;
        }

        percent += delta;
        percent = clamp_percent(percent);
        printf("ysim %d %d %f %f %f %f\n", net, error, available, delta, percent, percent * load / 100);
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
     *  CLI commands
     */

void cli_keypress(CLI *, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;
    struct _PowerManager::Message msg = { .type = _PowerManager::Message::M_KEY };
    pm->messages.push(msg);
}

void cli_mode(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "mode=%s%s", _PowerManager::mode_name(pm->get_mode()), cli->eol);
        return;
    }

    const int code = rlut(PowerManager::mode_lut, s);

    struct _PowerManager::Message msg = {
        .type = _PowerManager::Message::M_MODE,
        .value = code,
    };
    pm->messages.push(msg);
}

void cli_sim(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <off|power>%s", cmd->cmd, cli->eol);
        return;
    }

    if (!strcmp(s, "off"))
    {
        cli_print(cli, "sim off%s", cli->eol);
        pm->set_simulation(false);
        return;
    }

    int power = 0;
    const bool ok = cli_parse_int(s, & power, 0);
    if (!ok)
    {
        cli_print(cli, "error in power '%s'%s", s, cli->eol);
        return;
    }

    pm->set_simulation(true, power);
}

void cli_pulse(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <period>%s", cmd->cmd, cli->eol);
        return;
    }

    int period = 0;
    const bool ok = cli_parse_int(s, & period, 0);
    if (!ok)
    {
        cli_print(cli, "error in period '%s'%s", s, cli->eol);
        return;
    }

    pm->set_pulse(period);
}

    /*
     *
     */

void cli_forward(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    const char *args[CLI_MAX_ARGS+1] = { 0 };

    for (int idx = 0; idx <= CLI_MAX_ARGS; idx++)
    {
        const char *s = cli_get_arg(cli, idx);
        if (!s) break;
        args[idx] = s;
    }

    for (int idx = 0; args[idx]; idx++)
    {
        pm->forward(args[idx]);
        pm->forward(" ");
    }
    pm->forward("\r\n");
}

    /*
     *
     */

void cli_show(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    cli_print(cli, "power=%d%s", pm->get_power(), cli->eol);
    cli_print(cli, "percent=%d%s", pm->get_percent(), cli->eol);
    cli_print(cli, "phase=%d%s", pm->get_phase(), cli->eol);
    cli_print(cli, "mode=%s%s", pm->mode_name(pm->get_mode()), cli->eol);
}

    /*
     *
     */

void cli_phase(CLI *cli, CliCommand *cmd)
{
    ASSERT(cmd->ctx);
    _PowerManager *pm = (_PowerManager*) cmd->ctx;

    const char *s = cli_get_arg(cli, 0);
    if (!s)
    {
        cli_print(cli, "%s <phase>%s", cmd->cmd, cli->eol);
        return;
    }

    if (!strcmp("off", s))
    {
        pm->sim_phase(false, 0);
        return;
    }

    int phase = 0;
    const bool ok = cli_parse_int(s, & phase, 0);
    if (!ok)
    {
        cli_print(cli, "error in phase '%s'%s", s, cli->eol);
        return;
    }

    pm->sim_phase(true, phase);
}

    /*
     *
     */

static CliCommand cmds[] = {
    { "key", cli_keypress, "simulate key press", 0, 0 },
    { "mode", cli_mode, "mode <eco|on|off|base>", 0, 0 },
    { "sim", cli_sim, "sim <on|off|power>", 0, 0 },
    { "pulse", cli_pulse, "pulse <period>", 0, 0 },
    { "x", cli_forward, "forwards commands to send the stm32", 0, 0 },
    { "show", cli_show, "show status", 0, 0 },
    { "phase", cli_phase, "phase N|off # directly set triac phase", 0, 0 },
    { 0 },
};

    /*
     *  Install the application specific CLI commands
     */

static bool mc_cli_init(void *arg, Event *, Event::Queue *)
{
    PO_DEBUG("");
    ASSERT(arg);
    PowerManager *pm = (PowerManager*) arg;
    
    CLI *cli = (CLI*) Objects::objects->get("cli");
    ASSERT(cli);

    for (CliCommand *cmd = cmds; cmd->cmd; cmd++)
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
    GPIO *gpio = (GPIO*) Objects::objects->get("sw");
    ASSERT(gpio);
 
    Storage db("app");
    int32_t base = 20;
    int32_t load = 1000;
    int32_t target = -40;
    db.get("base", & base);
    db.get("load", & load);
    db.get("target", & target);
    PO_DEBUG("base=%d load=%d target=%d", (int) base, (int) load, (int) target);
 
    PowerControl *pc = PowerControl::create(load, target);
    PowerManager *pm = PowerManager::create(pc, leds, uart, gpio, base);
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
