
Mains Power Controller
----

### WORK IN PROGRESS!

----

## Warning!

Mains electricity can kill you. If you don't know what you are doing, don't do it. You have been warned.

----

![inside the enclosure](docs/IMG_20260207_121224Z.jpg)

I wanted a way to divert excess power from my solar panels into a load,
so I can use just excess solar power.
For example, to run an electric heater or a kettle.
This allows me to run an applicance at zero cost.

This project is based on an earlier one
[earlier one](https://www.rotwang.co.uk/projects/kettle.html)
and uses similar technology.
I designed a smart meter that sends out 
[MQTT](https://en.wikipedia.org/wiki/MQTT)
messages every second showing the total power import / export for my home/.
Using these messages I can apply power to a load that uses any excess solar power.

The design is simple : detect the zero-crossing point of the mains power waveform.
Turn on a [TRIAC](https://en.wikipedia.org/wiki/TRIAC),
controlling the phase to adjust the power applied to a load.

The [H11AA4](docs/H11AA1M.pdf) optocoupler is ideal for zero-crossing detection.
It has two back to back LEDs and a photo transistor. 
It provides Galvanic isolation, to separate the microcontroller from the mains power.
It just needs a single series resistor to set the max LED current.

The output circuit uses an opto-TRIAC [MOC3020](docs/MOC3020.pdf)
connected to a power TRAIC, the [BTA24](BTA24.pdf).
The opto device provides Galvanic isolation, the power TRIAC does the heavy lifting.

I use the ESP32 for lots of my projects and wanted to use it here, as it has
a built in WiFi module and supports MQTT.
However, my first attempt ran into a few problems.
I needed two things : a 
[Schmitt trigger](https://en.wikipedia.org/wiki/Schmitt_trigger)
input and a way to chain timer events to input and output signals.
It turned out that the ESP32C3 failed at both of these simple things.
The GPIOs do not have Schmitt inputs. I was surprised at this.
Secondly, the timers do not have event chaining.
I wanted the timing to be fully automatic so this ruled out the ESP32C3.
But I still wanted the WiFi support.

Without a Schmitt trigger the slow analog input signal causes spikes and multiple triggers like this :

![signal without a Schmitt trigger](docs/scr_20240117170940.png)

So I ended up with a hybrid solution. Not a great one, but a compromise.
I used 2 microcontrollers - one to provide the zero crossing detection and phase control,
the second to driver the control system and MQTT interface.
I settled on an STM32F1 as they are cheap, simple and I had one to hand.

![Using a thermal camera to check the temperature](docs/IMG_20260204_152732Z.jpg)

Using a thermal camera to check the temperature on the first prototype.
This fan was too small to cool the heatsink adequately.

----

## STM32 Controller

I write my embedded code using my [Panglos][Panglos] framework.
This allows me to develop target agnostic code and to reuse all my drivers and library code.
The STM32 code was very simple. I didn't need any OS support, 
so I could use Panglos directly as a bare metal system.
All it needed was UART support for communication with the ESP32,
GPIOs and some timer code.
I added simple GPIO and UART support to Panglos for the STM32F1 using direct control-register access.
I don't event need the HAL code.

The 
[STM32 code](https://github.com/DaveBerkeley/mains-power-control/stm32)
is on github.

The timer code was developed using interaction with [Claude AI](https://claude.ai/).
I'm interested in how AI can help write code, so I thought I'd experiment.
The timer code is a one-off, I don't want to write a portable library (unlike the UART and GPIO code). 
I just needed help chaining the GPIO and timers together.
It was interesting, requiring multiple iterations with Claude.
I was testing the code on real hardware.
I fed a low voltage sinewave into the opto detector as a signal source,
viewing the outputs on a 'scope.
I have a number of reservations about the use of AI in software. I think that we need to be careful.
But this was an experiment.

The result used 3 timers : one to trigger on the input from the zero-crossing detector.
This allows me to measure the period of the mains cycle.
The second timer provides the phase offset. This is a one-shot timer triggered by the first timer.
The period can be adjusted to control the phase of the output TRIAC.
This in turn triggers a third timer. This is another one-shot that provides the trigger pulse to the TRIAC.
Once configured the timers run independently of the CPU.
All you have to do is set the second timer's period to control the phase of the TRIAC and
therefore the power applied to the load.

The top trace is the analog output of the zero-crossing detector.
The input signal is a lower voltage for test purposes. In the real circuit the "on" periods will be shorter.
The second trace is the output of the second timer, showing the adjustable phase period.
The third trace is the TRAIC drive signal.

![Timer signals](docs/Screenshot_2026-01-03_17-11-09.png)

For development I added a CLI to the STM32 using my [CLI library][CLI library].
This allowed me to interact with the STM32, set the TRIAC phase, see the timing etc.
I was going to add a serial protocol for the ESP32 to talk to the STM32, but in the end
I just went with the CLI. It was already there and easy to use.

The device initialisation is data driven.  The STM32F1 is a relatively simple device. 
Each IO pin can have an ALT purpose, so the function is tied to a specific pin.
In the ESP32 you can assign any pin to be connected to any peripheral.
The config code simply sets the ALT function for GPIOs that are intialised eleswhere
by their drivers.

    #define DEBUG_UART (USART1)
    #define COMMS_UART (USART2)

    GPIO *led;

    typedef STM32F1_GPIO::IO IODEF;

    typedef struct {
        GPIO_TypeDef *port;
        uint32_t pin;
        IODEF io;
        GPIO **gpio;
    } IoDef;

    static const IODEF ALT_IN  = IODEF(IODEF::INPUT  | IODEF::ALT);
    static const IODEF ALT_OUT = IODEF(IODEF::OUTPUT | IODEF::ALT);

    static const IoDef gpios[] = {
        {   GPIOC, 13, IODEF::OUTPUT, & led, }, // LED
        {   GPIOA, 9,  ALT_OUT, }, // UART Tx
        {   GPIOA, 10, ALT_IN,  }, // UART Rx
        {   GPIOA, 2,  ALT_OUT, }, // Comms Tx
        {   GPIOA, 3,  ALT_IN,  }, // Comms Rx
        {   GPIOA, 0,  ALT_IN,  }, // TIM2 counter/timer input
        {   GPIOA, 6,  ALT_OUT, }, // TIM3 phase output
        {   GPIOB, 6,  ALT_OUT, }, // TIM4 triac output
        { 0 },
    };

    void init_gpio(const IoDef *gpios)
    {
        for (const IoDef *def = gpios; def->port; def++)
        {
            STM32F1_GPIO::IO io = def->io;
            if (!def->gpio) io = IODEF(io | IODEF::INIT_ONLY);
            GPIO *gpio = STM32F1_GPIO::create(def->port, def->pin, io);
            if (def->gpio) *def->gpio = gpio;
        }
    }

Once the target specific initialisation is done, the devices 
(with the exception of the timer code),
are all abstracted, so the code is OS and target agnostic.

----

## ESP32 Controller

The main control system runs on the ESP32. I used an 
[ESP32C3 XIAO](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
from [Seeed Studio](https://www.seeedstudio.com/).
It uses my [Panglos][Panglos] framework on top of FreeRTOS.
The controller does the following :

* controls the STM32 via a UART
* provides a UI via a push button and 2 smart LEDs
* connects to the home WiFi
* subscribes to MQTT messages from my smart meter
* monitors the TRIAC temperature using a [DS18B20](docs/DS18B20.pdf) [OneWire](https://en.wikipedia.org/wiki/1-Wire) sensor.
* controls a cooling fan using a GPIO pin
* regulates the power applied to the load by controlling the TRIAC phase
* handles error situations : eg. loss of WiFi, overtemperature etc.

For the LEDs I used the [WS2812B](docs/WS2812B.pdf) smart LED.
I already had drivers for this device in [Panglos][Panglos] that wrap the ESPIDF RMT library.
I had to add a [OneWire driver](https://github.com/DaveBerkeley/panglos/blob/master/src/esp32/one_wire_bitbang.cpp).
There is a driver in ESPIDF that also uses the RMT device, but I was already using this for the LEDs.
So I had to use a bit-banged driver instead.
The [DS18B20 driver](https://github.com/DaveBerkeley/panglos/blob/master/src/drivers/ds18b20.cpp)
works with either OneWire driver.
It follows the [Panglos][Panglos] design philosophy of removing all hardware and OS dependency from the code.
It also abstracts the temperature sensor so that you can unit test the control code without the hardware.

The temperature control code is very simple.
It simply turns on a fan when the temperature rises above a set point.
It turns the fan off when the teperature falls below a different set point.
It also proves an alarm() output for over-temperature.
As both the Temperature sensor and the GPIO used to control the fan are both abstracted, 
they are trivially simple to test.

The ESP32 also runs my [CLI library][CLI library]. This allows full control of the configuration of the system.
It also provides a number of test modes : for example you can simulate the device temperature to check the 
control systems are working.
The CLI is available over the USB/UART interface.
The ESP32 also runs a CLI server over the WiFi connection on port 6668, so you can telnet to the device remotely.
This makes test and monitoring very simple.
To monitor the device you can, for example, 
turn on 'verbose' mode for the app and log all debug output to the selected network CLI.

    > verbose app 1
    > log
    703220 main DEBUG src/mains_control.cpp +472 set_phase() : phase=0 percent=0 power=273 t=15
    703302 main DEBUG src/mains_control.cpp +472 set_phase() : phase=0 percent=0 power=271 t=15
    703404 main DEBUG src/mains_control.cpp +472 set_phase() : phase=0 percent=0 power=270 t=15
    703507 main DEBUG src/mains_control.cpp +472 set_phase() : phase=0 percent=0 power=270 t=15
    703609 main DEBUG src/mains_control.cpp +472 set_phase() : phase=0 percent=0 power=272 t=15

The net CLI code supports multiple clients, so you can log in on one term and tail the log in another :

    echo -e "log\n" | nc pwr1.local 6668 

The code supports [mDMS](https://en.wikipedia.org/wiki/Multicast_DNS) so you can configure the unit 
with a friendly name. In this case 'pwr1'.

The [ESP32 code](esp32) is in github. 
The main control code is in [esp32/src/mains_control.cpp](esp32/src/mains_control.cpp).

The device initialisation is more complex than the STM32 case. It uses the
panglos::init_devices() code. This takes an array of Device descriptions
and initialises them in order, taking into account any device dependency.
In this case the ds18b20 temperature sensor needs the OneWire device to be 
created first.
By convention the Device array board_devs is used to specify all the devices.
The function board_init() is also called after the main system is up.

The board_init() function registers the network CLI handler.
It then fetches the MQTT config from non-volatile storage and calls
mains_control_init().
This function creates the temperature controller (reponsible for fan control and alarms),
the power control (responsible for power / phase control)
and the power manager, that runs the overall system.
A 10ms timer is used to read the UI button. 
Callbacks drive the power manager :  MQTT callbacks provide smart meter data,
on_idle() provides an idle hook into the CLI.
Commands are added to the CLI.

    static const GPIO_DEF button_def = { GPIO_NUM_8, ESP_GPIO::IP | ESP_GPIO::PU, false };
    static const GPIO_DEF fan_def = { GPIO_NUM_6, ESP_GPIO::OP, true };
    static const GPIO_DEF lamp_def = { GPIO_NUM_10, ESP_GPIO::OP | ESP_GPIO::OD, false };

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
        Device("lamp", 0, gpio_init, (void*) & lamp_def),
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

----

User Interface
====

The user interface consists of a single illuminated push button and 2 smarts LEDs.
One LED indicates Mode, and other shows metered import / export.
Imports are shown in Red, exports in Green, the brightness reflects the number of Watts.
The button steps through the Modes : On, Off, Eco and Base. On and Off are self explanatory.
Eco Mode will use any excess export power and divert it to the load.
Base Mode is like Eco Mode but it will always provide a minimum level of power to the load.
The default is 20%. 
This can be used, for example, to power a kettle. It uses any solar power is possible,
but if not enough is available it will still boil the water : it will just take longer.

Error modes are shown by alternating flashing of the 2 LEDs. Over temperature as Red flashing LEDs,
WiFi or MQTT error as Blue. In error mode the power to the load will be reduced to zero
until the error clears. If the network is down for too long the unit will reboot 
and try to connect again. The MQTT driver will attempt to reconnect if it loses connection.

----

Schematics and PCB
====

I still haven't built any surface mount boards, so I am stuck using through-hole designs. 
They do have a distinct 1980s feel to them.
I'm also using bought-in microcontroller modules rather than using discrete parts.
So the board is much bigger than it should be.
The schematic shows the zero-crossing detector, the TRIAC drive, the fan control, LEDs
and the 2 microcontrollers.
A commercial product would not look anything like this.

![Schematic](docs/Screenshot_2026-01-26_17-32-56.png)

The PCB layout was done using [KiCad](https://www.kicad.org/).
The files are in [github](kicad/mains-power-control/).
The PCB looks like this :

![PCB](docs/Screenshot_2026-01-26_17-35-11.png)

In version 1.1 of the board I placed the LEDs and switch housing on the PCB itself, 
rather than have a flying lead.
I added a FET switch to control the fan and a OneWire interface for the temperature sensor.
I left on test points for the STM32.
The XIAO ESP32C3 has an antenna socket. The simple ESP32C3 supermini boards do not
and this can limit the WiFi reception. 
There is also a ESP32C3 supermini plus that has an antenna socket.
I've been using this for other projects.

The circuit is missing a proper snubber circuit for the TRIACs.
The design would fail EMC regulations. I need to add this.

----

Enclosure
====

The enclosure design is written in 
[SCAD](https://openscad.org/).

![Enclosure CAD view](docs/Screenshot_2026-01-30_13-13-38.png)

The unit has an IEC power inlet, a UK style 13-Amp plug socket, the control PCB
connected to the switch and LEDs, the TRAIC mounted on a heatsink and a cooling fan.

There are air inlets on 2 sides, to allow the fan to produce a flow of air across the heatsink.
The inlets are designed with a kink in the path to prevent you from pushing things through them.

[Panglos]: https://github.com/DaveBerkeley/panglos 
[CLI library]: https://github.com/DaveBerkeley/cli

