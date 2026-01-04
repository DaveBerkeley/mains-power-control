
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/logger.h"

#include "cli/src/cli.h"

#include "gpio.h"
#include "uart.h"
#include "timer.h"
#include "cli.h"

using namespace panglos;

#if defined(STM32F103xB)
    // blue-pill board!
#endif

#define DEBUG_UART (USART1)

static GPIO *led;

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    STM32_GPIO::IO io;
    GPIO **gpio;
} IoDef;

static const IoDef gpios[] = {
    {   GPIOC, 13, STM32_GPIO::OUTPUT, & led, }, // LED
    {   GPIOA, 9, STM32_GPIO::IO(STM32_GPIO::OUTPUT | STM32_GPIO::ALT), }, // UART Tx
    {   GPIOA, 10, STM32_GPIO::IO(STM32_GPIO::INPUT | STM32_GPIO::ALT), }, // UART Rx
    {   GPIOA, 0, STM32_GPIO::IO(STM32_GPIO::INPUT | STM32_GPIO::ALT), }, // TIM2 counter/timer input
    {   GPIOA, 6, STM32_GPIO::IO(STM32_GPIO::OUTPUT | STM32_GPIO::ALT), }, // TIM3 phase output
    {   GPIOB, 6, STM32_GPIO::IO(STM32_GPIO::OUTPUT | STM32_GPIO::ALT), }, // TIM4 triac output
    { 0 },
};

void init_gpio(const IoDef *gpios)
{
    for (const IoDef *def = gpios; def->port; def++)
    {
        STM32_GPIO::IO io = def->io;
        if (!def->gpio) io = STM32_GPIO::IO(io | STM32_GPIO::INIT_ONLY);
        GPIO *gpio = STM32_GPIO::create(def->port, def->pin, io);
        if (def->gpio) *def->gpio = gpio;
    }
}

volatile uint32_t tick;

    /*
     *
     */

extern "C" void SysTick_Handler(void)
{
    tick += 1;
    if ((tick % 100) == 0)
        if (led)
            led->toggle();
}

    /*
     *
     */

static const char *banner[] = {
    "",
    " ____                   _  ___  ____  ",
    "|  _ \\ __ _ _ __   __ _| |/ _ \\/ ___| ",
    "| |_) / _` | '_ \\ / _` | | | | \\___ \\ ",
    "|  __/ (_| | | | | (_| | | |_| |___) |",
    "|_|   \\__,_|_| |_|\\__, |_|\\___/|____/ ",
    "                  |___/               ",
};

    /*
     *
     */

int main(void)
{
    // initialise all the GPIO pins
    init_gpio(gpios);

    // create the main UART
    UART *uart = STM32_UART::create(DEBUG_UART, 32);
    // create a logger
    logger = new Logging(S_DEBUG, 0);
    logger->add(uart, S_DEBUG, 0);

    for (const char **t = banner; *t; t++)
    {
        uart->tx(*t, strlen(*t));
        uart->tx("\r\n", 2);
    }
    const char *text = "PanglOS on STM32 " __TIME__ " " __DATE__ "\r\n";
    uart->tx(text, strlen(text));
 
    const int err = SysTick_Config(SystemCoreClock / 1000);
    ASSERT(!err);

    Timers_Init();

    TIM4_SetPulseWidth(100);

    uint32_t width = 1;

    static CLI cli { 0 };
    init_cli(& cli, uart);

    while (true)
    {
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

        while (!capture_ready)
            ;
        capture_ready = 0;
        TIM3_SetPulseWidth(width);

        width += 1;
        if (width >= 9000)
            width = 1;
    }

    return 0;
}

//  FIN
