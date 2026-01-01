
#include <assert.h>
#include <stdarg.h>

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/logger.h"

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

#define LED_PORT (GPIOC)
#define LED_PIN (13)
#define UART_RX_PORT (GPIOA)
#define UART_RX_PIN (10)
#define UART_TX_PORT (GPIOA)
#define UART_TX_PIN (9)

static GPIO *led;

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

int main(void)
{
    // initialise the UART1 GPIO pins
    STM32_GPIO::create(UART_TX_PORT, UART_TX_PIN, STM32_GPIO::IO(STM32_GPIO::OUTPUT | STM32_GPIO::ALT)); // Tx
    STM32_GPIO::create(UART_RX_PORT, UART_RX_PIN, STM32_GPIO::IO(STM32_GPIO::INPUT | STM32_GPIO::ALT)); // Rx
    // create the main UART
    UART *uart = STM32_UART::create(USART1);

    logger = new Logging(S_DEBUG, 0);
    logger->add(uart, S_DEBUG, 0);

    PO_DEBUG("PanglOs STM32 System"); 
    int i = 123;
    PO_DEBUG("hello %d", i);
 
    //NVIC_EnableIRQ(TIM2_IRQn);
    led = STM32_GPIO::create(LED_PORT, LED_PIN, STM32_GPIO::OUTPUT);

    PO_DEBUG("");
    int err = SysTick_Config(SystemCoreClock / 1000);
    PO_DEBUG("");
#if 0
    if (err)
    {
        assert(0);
    }
#endif

    while (true)
    {
        ms_delay(1500);
        //led->toggle();
        PO_DEBUG("");
    }

    return 0;
}
