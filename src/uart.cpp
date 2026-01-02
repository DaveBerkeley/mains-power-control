
#include "panglos/debug.h"

#include "uart.h"

class _STM32_UART : public STM32_UART
{
    USART_TypeDef *base;
    static uint32_t ck_enabled;

    void enable_port_power()
    {
        uint32_t pwr_mask = 0;
        volatile uint32_t *reg = 0;

        switch ((uint32_t) base)
        {
            case USART1_BASE : pwr_mask = RCC_APB2ENR_USART1EN; reg = & RCC->APB2ENR; break;
            case USART2_BASE : pwr_mask = RCC_APB1ENR_USART2EN; reg = & RCC->APB1ENR; break;
            case USART3_BASE : pwr_mask = RCC_APB1ENR_USART3EN; reg = & RCC->APB1ENR; break;
            default : ASSERT(0);
        }
        if (!(ck_enabled & pwr_mask))
        {
            ASSERT(reg);
            *reg |= pwr_mask;
            ck_enabled |= pwr_mask;
        }
    }

public:
    _STM32_UART(USART_TypeDef *_base)
    :   base(_base)
    {
    }

    void init(void)
    {
        enable_port_power();
 
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
        // TODO
        ASSERT(0);
        return n;
    }
};

uint32_t _STM32_UART::ck_enabled = 0;

panglos::UART *STM32_UART::create(USART_TypeDef *base)
{
    _STM32_UART *uart = new _STM32_UART(base);
    uart->init();
    return uart;
}

//  FIN
