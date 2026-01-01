
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/io.h"
#include "panglos/logger.h"


#include "panglos/drivers/uart.h"

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

class STM32_UART : public UART
{
    USART_TypeDef *base;
public:
    STM32_UART(USART_TypeDef *_base)
    :   base(_base)
    {
    }

    void init(void)
    {
        // Enable clocks for GPIOA and USART1
        //RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
 
        // Configure PA9 (TX) as alternate function push-pull
        // CNF9[1:0] = 10 (AF push-pull), MODE9[1:0] = 11 (50MHz)
        GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
        GPIOA->CRH |= GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9;
 
        // Configure PA10 (RX) as input floating
        // CNF10[1:0] = 01 (floating input), MODE10[1:0] = 00 (input)
        GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
        GPIOA->CRH |= GPIO_CRH_CNF10_0;

        // Configure USART1
        // Assuming 8MHz clock, baud rate = 115200
        // BRR = 8000000 / 115200 = 69.44 ≈ 0x45
        base->BRR = 0x45;  // For 115200 baud @ 8MHz
       
        // Enable USART, TX
        base->CR1 = USART_CR1_TE | USART_CR1_UE;
    }

    // Send a single character
    void putchar(char c)
    {
        // Wait until TX buffer is empty
        while(!(base->SR & USART_SR_TXE))
            ;
        base->DR = c;
    }

    // Send a string
    void xSendString(const char *str)
    {
        while(*str)
        {
            putchar(*str++);
        }
    }

    virtual int tx(const char* data, int n)
    {
        for (int i = 0; i < n; i++)
        {
            putchar(*data++);
        }
        return n;
    }

    virtual int rx(char* data, int n) override
    {
        ASSERT(0);
    }
};

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
    STM32_UART uart(USART1);
    uart.init();

    logger = new Logging(S_DEBUG, 0);

    logger->add(& uart, S_DEBUG, 0);
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
    }

    return 0;
}
