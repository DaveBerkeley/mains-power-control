
#include "panglos/debug.h"

#include "gpio.h"

#if defined(STM32F1)

#define ENABLE_GPIO_CLOCK (RCC->APB2ENR |= RCC_APB2ENR_IOPCEN)

class _STM32_GPIO : public STM32_GPIO
{
private:
    static bool ck_enabled;

    GPIO_TypeDef *base;
    uint32_t mask;

public:
    _STM32_GPIO(GPIO_TypeDef *_base, uint32_t pin, IO io)
    :   base(_base),
        mask(1 << pin)
    {
        if (!ck_enabled)
        {
            ENABLE_GPIO_CLOCK;
            ck_enabled = true;
        }

        // 4 bits in control reg for cfg[1:0] and mode[1:0]
        // TODO : allow access to Speed setting? eg Max speed 10/2/50 MHz
        const uint8_t mode = (io & INPUT) ? 0 : 0x01;

        uint8_t cfg = 0;

        if (io & INPUT)
        {
            if (io & (PULL_UP | PULL_DOWN))
            {
                cfg |= 0x02;
            }
        }

        if (io & OUTPUT)
        {
            if (io & OPEN_DRAIN)
            {
                cfg |= 0x01;
            }
            if (io & ALT)
            {
                cfg |= 0x02;
            }
        }

        const uint8_t d = (cfg << 2) | mode;
        const uint8_t cfg_mask = 0xf;
        const uint8_t shift = (pin % 8) * 4;
        // CRL for pins 0..7, CRH for 8..15
        volatile uint32_t *reg = (pin < 8) ? & base->CRL : & base->CRH;
        *reg &= ~(cfg_mask << shift);
        *reg |= d << shift;
    }

    virtual void set(bool state) override
    {
        volatile uint32_t *reg = state ? & base->BSRR : & base->BRR;
        *reg = mask;
    }
    virtual bool get() override
    {
        return base->IDR & mask;
    }
    virtual void toggle() override
    {
        set(!get());
    }
};

bool _STM32_GPIO::ck_enabled = false;

panglos::GPIO *STM32_GPIO::create(GPIO_TypeDef *_base, uint32_t _pin, IO io)
{
    return new _STM32_GPIO(_base, _pin, io);
}

#endif  //  STM32F1

