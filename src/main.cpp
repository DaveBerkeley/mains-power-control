
#include <assert.h>

#include "panglos/debug.h"

//#include "panglos/drivers/uart.h"

#include "gpio.h"

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

#define LED_PORT (GPIOC)
#define LED_PIN (13)

int main(void)
{
    NVIC_EnableIRQ(TIM2_IRQn);
    GPIO *gpio = STM32_GPIO::create(LED_PORT, LED_PIN, STM32_GPIO::OUTPUT);

    while (true)
    {
        ms_delay(500);
        gpio->toggle();
    }

    return 0;
}
