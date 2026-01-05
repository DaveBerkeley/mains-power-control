
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/logger.h"
#include "panglos/stm32/gpio_f1.h"
#include "panglos/stm32/uart_f1.h"

#include "cli/src/cli.h"

//#include "uart.h"
#include "timer.h"
#include "cli.h"

using namespace panglos;

#if defined(STM32F103xB)
    // blue-pill board!
#endif

    /*
     *  IO configuration
     */

#define DEBUG_UART (USART1)
#define COMMS_UART (USART2)

GPIO *led;

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    STM32F1_GPIO::IO io;
    GPIO **gpio;
} IoDef;

static const STM32F1_GPIO::IO ALT_IN = STM32F1_GPIO::IO(STM32F1_GPIO::INPUT | STM32F1_GPIO::ALT);
static const STM32F1_GPIO::IO ALT_OUT = STM32F1_GPIO::IO(STM32F1_GPIO::OUTPUT | STM32F1_GPIO::ALT);

static const IoDef gpios[] = {
    {   GPIOC, 13, STM32F1_GPIO::OUTPUT, & led, }, // LED
    {   GPIOA, 9,  ALT_OUT, }, // UART Tx
    {   GPIOA, 10, ALT_IN, }, // UART Rx
    {   GPIOA, 2,  ALT_OUT, }, // Comms Tx
    {   GPIOA, 3,  ALT_IN, }, // Comms Rx
    {   GPIOA, 0,  ALT_IN, }, // TIM2 counter/timer input
    {   GPIOA, 6,  ALT_OUT, }, // TIM3 phase output
    {   GPIOB, 6,  ALT_OUT, }, // TIM4 triac output
    { 0 },
};

void init_gpio(const IoDef *gpios)
{
    for (const IoDef *def = gpios; def->port; def++)
    {
        STM32F1_GPIO::IO io = def->io;
        if (!def->gpio) io = STM32F1_GPIO::IO(io | STM32F1_GPIO::INIT_ONLY);
        GPIO *gpio = STM32F1_GPIO::create(def->port, def->pin, io);
        if (def->gpio) *def->gpio = gpio;
    }
}

volatile uint32_t tick;
bool flash = true;

    /*
     *
     */

extern "C" void SysTick_Handler(void)
{
    tick += 1;
    if (flash)
        if ((tick % 100) == 0)
            if (led)
                led->toggle();
}

    /*
     *
     */

// echo "PanglOS" | figlet | sed 's/\\/\\\\/g'

static const char *banner[] = {
    "",
    " ____                   _  ___  ____  ",
    "|  _ \\ __ _ _ __   __ _| |/ _ \\/ ___| ",
    "| |_) / _` | '_ \\ / _` | | | | \\___ \\ ",
    "|  __/ (_| | | | | (_| | | |_| |___) |",
    "|_|   \\__,_|_| |_|\\__, |_|\\___/|____/ ",
    "                  |___/               ",
    "",
    "PanglOS on STM32F1 " __TIME__ " " __DATE__ "",
    "https://github.com/DaveBerkeley/panglos",
    0,
};

    /*
     *
     */

void poll_comms(UART *uart)
{
    //  TODO : could just use the CLI
}

    /*
     *
     */

int main(void)
{
    // initialise all the GPIO pins
    init_gpio(gpios);

    // create the main UART
    UART *uart = STM32F1_UART::create(DEBUG_UART, 32);
    // create a logger
    logger = new Logging(S_DEBUG, 0);
    logger->add(uart, S_DEBUG, 0);

    for (const char **t = banner; *t; t++)
    {
        uart->tx(*t, strlen(*t));
        uart->tx("\r\n", 2);
    }
 
    const int err = SysTick_Config(SystemCoreClock / 1000);
    ASSERT(!err);

    Timers_Init();

    static CLI cli { 0 };
    init_cli(& cli, uart);

    UART *comms = STM32F1_UART::create(COMMS_UART, 32);

    while (true)
    {
        poll_comms(comms);

        char buff[16];
        int n = uart->rx(buff, sizeof(buff));
        for (int i = 0; i < n; i++)
        {
            char c = buff[i];
            switch (c)
            {
                case '\r' : c = '\n'; break;
                case '\n' : continue;
                default : cli_print(& cli, "%c", c); break;
            }
            cli_process(& cli, c);
        }
    }

    return 0;
}

//  FIN
