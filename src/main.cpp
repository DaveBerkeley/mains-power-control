
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/io.h"
#include "panglos/logger.h"


#include "panglos/drivers/uart.h"

#include "gpio.h"
#include "uart.h"

#if defined(ASSERT)
#undef ASSERT
#define ASSERT(x) assert(x)
#endif

using namespace panglos;

#if defined(STM32F103xB)
    // blue-pill board!
#endif

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

    /*
     *  Logging interface
     */

const LUT Severity_lut[] = {
    // Logging Severity
    {   "NONE", S_NONE, },
    {   "CRITICAL", S_CRITICAL, },
    {   "ERROR", S_ERROR, },
    {   "WARNING", S_WARNING, },
    {   "NOTICE", S_NOTICE, },
    {   "INFO", S_INFO, },
    {   "DEBUG", S_DEBUG, },
    { 0, 0 },
};

uint32_t get_time(void) { return 0; }
const char *get_task(void) { return ""; }

void Error_Handler(void)
{
    while (true) 
        ;
}

extern "C" bool arch_in_irq()
{
    return false;
}

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

#define LED_PORT (GPIOC)
#define LED_PIN (13)

static GPIO *led;

void SysTick_Handler(void)
{
    if (led)
        led->toggle();
}

int main(void)
{
    STM32_GPIO::create(GPIOA, 9, STM32_GPIO::IO(STM32_GPIO::OUTPUT |  STM32_GPIO::ALT)); // Tx
    //STM32_GPIO::create(GPIOA, 10, STM32_GPIO::IO(STM32_GPIO::INPUT |  STM32_GPIO::ALT)); // Rx

    UART *uart = STM32_UART::create(USART1);

    logger = new Logging(S_DEBUG, 0);

    logger->add(uart, S_DEBUG, 0);
    PO_DEBUG("PanglOs STM32 System");
 
    int i = 123;
    PO_DEBUG("hello %d", i);
 
    // Send test string
    //SysTick->CTRL = 0; // disable systick (claude)
    //SystemInit();
    //NVIC_EnableIRQ(TIM2_IRQn);
    led = STM32_GPIO::create(LED_PORT, LED_PIN, STM32_GPIO::OUTPUT);

    //int err = SysTick_Config(SystemCoreClock / 100000);
#if 0
    if (err)
    {
        assert(0);
    }
    //NVIC_EnableIRQ(SysTick_IRQn);
#endif

    while (true)
    {
        ms_delay(500);
        led->toggle();
        //uart.SendString("Hello from STM32F1!\r\n");
        PO_DEBUG("");
    }

    return 0;
}
