
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/logger.h"

#include "gpio.h"
#include "uart.h"
#include "timer.h"

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
    {   GPIOA, 9, STM32_GPIO::IO(STM32_GPIO::OUTPUT | STM32_GPIO::ALT), }, // UART Tx
    {   GPIOA, 10, STM32_GPIO::IO(STM32_GPIO::INPUT | STM32_GPIO::ALT), }, // UART Rx
    {   GPIOC, 13, STM32_GPIO::OUTPUT, & led, }, // LED
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

static void ms_delay(int ms)
{
    while (ms-- > 0)
    {
        for (volatile int x=500; x--; )
        {
            __asm("nop");
        }
    }
}

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

int main(void)
{
    // initialise the GPIO pins
    init_gpio(gpios);

    // create the main UART
    UART *uart = STM32_UART::create(DEBUG_UART);
    // create a logger
    logger = new Logging(S_DEBUG, 0);
    logger->add(uart, S_DEBUG, 0);

    for (const char **t = banner; *t; t++)
    {
        uart->tx(*t, strlen(*t));
        uart->tx("\r\n", 2);
    }
    const char *text = "PanglOS on STM32\r\n";
    uart->tx(text, strlen(text));
 
    PO_DEBUG("SysTick_Config()");
    const int err = SysTick_Config(SystemCoreClock / 1000);
    ASSERT(!err);

    PO_DEBUG("Timers_Init()");
    Timers_Init();

    //TIM3_SetPulseWidth(9000);
    TIM3_SetPulseWidth(1);

    while (true)
    {
        ms_delay(100);
        PO_DEBUG("%lu", period_us);


//PO_DEBUG("TIM3: PSC=%lu ARR=%lu CCR1=%lu CCMR1=0x%04lX CCER=0x%04lX CR1=0x%04lX", TIM3->PSC, TIM3->ARR, TIM3->CCR1, TIM3->CCMR1, TIM3->CCER, TIM3->CR1);
//PO_DEBUG("RCC_CFGR: 0x%08lX, APB1 prescaler: %lu", RCC->CFGR, (RCC->CFGR & RCC_CFGR_PPRE1) >> 8);
//PO_DEBUG("TIM3: PSC=%lu ARR=%lu CCR1=%lu CNT=%lu", TIM3->PSC, TIM3->ARR, TIM3->CCR1, TIM3->CNT);
    }

    return 0;
}

//  FIN
