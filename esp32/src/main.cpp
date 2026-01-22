
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <endian.h>

#if defined(ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

//#include "gtest/gtest.h"

#if defined(ESP32)
// espidf framework
#include <driver/gpio.h>
#include <driver/ledc.h>

#include "nvs_flash.h"
#endif

//  Panglos

#include "panglos/debug.h"
#include "panglos/time.h"
#include "panglos/object.h"
#include "panglos/thread.h"
#include "panglos/logger.h"
#include "panglos/device.h"
#include "panglos/io.h"
#include "panglos/mqtt.h"
#include "panglos/storage.h"
#include "panglos/event_queue.h"
#include "panglos/verbose.h"
#include "panglos/watchdog.h"
#include "panglos/network.h"

#include "panglos/arch.h"
#include "panglos/hal.h"

#include "cli/src/cli.h"

#include "panglos/app/cli.h"
#include "panglos/app/devices.h"
#include "panglos/app/event.h"

using namespace panglos;

#define esp_error(err)    ASSERT_ERROR((err) == ESP_OK, "err=%s", lut(panglos::err_lut, (err)));

__attribute__((weak)) void board_init() { PO_ERROR("Not defined"); }

extern panglos::Device *board_devs;

#if defined(ESP32C3_XIAO)

#define WIFI
#define MQTT

#endif  //  ESP32C3_XIAO

extern "C" void po_log(Severity s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (logger)
    {
        logger->log(s, fmt, ap);
    }
    else
    {
        vprintf(fmt, ap);
    }
    va_end(ap);
}

    /*
     *
     */

class StdOut : public Out
{
    virtual int tx(const char* data, int n) override
    {
        printf("%.*s", n, data);
        return n;
    }
};

    /*
     *
     */

static bool init_devices()
{
    ASSERT(Objects::objects);

    panglos::List<Device*> done(Device::get_next);

    bool verbose = true;

    //  First initialise the network (if any)

    void start_network(); // TODO : this should be in a header file
    start_network();

    if (Objects::objects->get("net"))
    {
        MqttClient *mqtt = new MqttClient;
        Objects::objects->add("mqtt", mqtt);
    }

    {
        //  Initialise any board specific devices
        panglos::List<Device*> devices(Device::get_next);

        for (Device *d = board_devs; d->name; d++)
        {
            devices.push(d, 0);
        }

        const bool ok = Device::init_devices(devices, verbose, & done);
        if (!ok)
        {
            return false;
        }
    }

    // Start mDNS?
    if (Objects::objects->get("net"))
    {
        Storage db("net");

        char name[64];
        size_t size = sizeof(name);
        if (db.get("hostname", name, & size))
        {
            Network::start_mdns(strdup(name));
        }
    }
 
    board_init();
    return true;
}

    /*
     *
     */

    //  Start the MQTT task
    //  note: any subscriptions made after this will not be acted upon.
static void start_mqtt(MqttClient *mqtt)
{
    // Get mqtt host name from db
    Storage db("mqtt");
    char host[24] = { 0 };
    size_t s = sizeof(host);
    if (!db.get("host", host, & s))
    {
        PO_ERROR("No mqtt.host in db %s", host);
    }
    else
    {
        static char uri[32];
        snprintf(uri, sizeof(uri), "mqtt://%s", host);
        mqtt->app_start(uri);
    }
}

static void start_app()
{
    if (!init_devices())
    {
        PO_ERROR("unable to create devices");
        return;
    }

    //  Initialise the MQTT subscriptions
    MqttClient *mqtt = (MqttClient*) Objects::objects->get("mqtt");
    if (mqtt)
    {
        start_mqtt(mqtt);
        Time::sleep(3);
    }

    extern const char *banner;
    Objects::objects->add("banner", (void*) banner);

    // Define main program execution
    Objects::objects->add("app", (void*) run_cli);
}

    /*
     *
     */

extern "C" void app_main(void)
{
#if defined(ESP32)
    esp_err_t err;

    err = nvs_flash_init();
    esp_error(err);
#endif

    // must be a real mutex here. The TASK_LOCK Mutex will block eg network, timers
    panglos::Mutex *mutex = panglos::Mutex::create(panglos::Mutex::SYSTEM);
    logger = new Logging(S_DEBUG, mutex);

    StdOut output;
    logger->add(& output, S_DEBUG, 0);
    PO_DEBUG("PanglOs ESP32 System");

    verbose_init();

    Objects::objects = Objects::create(true);
    ASSERT(Objects::objects);

    EvQueue *eq = new EvQueue;
    Objects::objects->add("event_queue", eq);

    start_app();

    panglos::Mutex *m = panglos::Mutex::create(panglos::Mutex::SYSTEM);
    Watchdog *watchdog = Watchdog::create(m);
    Objects::objects->add("watchdog", watchdog);

    void (*app)() = (void (*)()) Objects::objects->get("app");
    ASSERT(app);

    app();
}

//  FiN
