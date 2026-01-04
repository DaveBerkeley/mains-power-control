
#include "panglos/debug.h"

#include "uart.h"

class _STM32_UART;

static _STM32_UART *uarts[3];

class _STM32_UART : public STM32_UART
{
    USART_TypeDef *base;
    static uint32_t ck_enabled;
    char *rx_buff;
    int32_t rx_size;
    int32_t rx_idx;

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

    static IRQn_Type base_to_irq(USART_TypeDef *base)
    {
        switch ((uint32_t) base)
        {
            case USART1_BASE : return USART1_IRQn;
            case USART2_BASE : return USART2_IRQn;
            case USART3_BASE : return USART3_IRQn;
        }
        ASSERT(0);
        return TAMPER_IRQn; // never gets here
    }

public:
    _STM32_UART(USART_TypeDef *_base, uint32_t _rx_size)
    :   base(_base),
        rx_size(_rx_size)
    {
        enable_port_power();
 
        // Configure USART1
        // Assuming 8MHz clock, baud rate = 115200
        // BRR = 8000000 / 115200 = 69.44 ≈ 0x45
        base->BRR = 0x45;  // For 115200 baud @ 8MHz
 
        // Enable USART, TX
        base->CR1 = USART_CR1_TE | USART_CR1_UE;

        // Enable RX interrupts
        if (rx_size)
            base->CR1 |= USART_CR1_RXNEIE | USART_CR1_RE;

        switch ((uint32_t) base)
        {
            case USART1_BASE : uarts[0] = this; break;
            case USART2_BASE : uarts[1] = this; break;
            case USART3_BASE : uarts[2] = this; break;
        }

        IRQn_Type irq = base_to_irq(base);
        NVIC_EnableIRQ(irq);
        NVIC_SetPriority(irq, 0);
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

    void on_interrupt()
    {
//    if (TIM2->SR & TIM_SR_CC1IF)
//    {
//        TIM2->SR &= ~TIM_SR_CC1IF;
//        period_us = TIM2->CCR1;
//        capture_ready = 1;
//    }
    }

    static void on_interrupt(uint32_t idx)
    {
        _STM32_UART *uart = uarts[idx];
        ASSERT(uart);
        uart->on_interrupt();
    }
};

uint32_t _STM32_UART::ck_enabled = 0;

panglos::UART *STM32_UART::create(USART_TypeDef *base, uint32_t rx_size)
{
    return new _STM32_UART(base, rx_size);
}

extern "C" void USART1_IRQHandler(void)
{
    _STM32_UART::on_interrupt(0);
}

extern "C" void USART2_IRQHandler(void)
{
    _STM32_UART::on_interrupt(1);
}

extern "C" void USART3_IRQHandler(void)
{
    _STM32_UART::on_interrupt(2);
}

//  FIN
