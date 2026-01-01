
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

class STM32_UART
{
    USART_TypeDef *base;
public:
    STM32_UART(USART_TypeDef *_base)
    :   base(_base)
    {
    }

    void Init(void)
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
    void SendChar(char c)
    {
        // Wait until TX buffer is empty
        while(!(base->SR & USART_SR_TXE))
            ;
        base->DR = c;
    }

    // Send a string
    void SendString(const char *str)
    {
        while(*str)
        {
            SendChar(*str++);
        }
    }
};

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
    uart.Init();
    
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
        uart.SendString("Hello from STM32F1!\r\n");
    }

    return 0;
}
