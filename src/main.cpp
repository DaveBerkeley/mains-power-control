
#include <assert.h>

#include "panglos/debug.h"

//#include "panglos/drivers/uart.h"
#include "panglos/drivers/gpio.h"

#if defined(ASSERT)
#undef ASSERT
#define ASSERT(x) assert(x)
#endif

using namespace panglos;

#if defined(STM32F1)

#if defined(STM32F103xB)
    // blue-pill board!
#endif

#include "stm32f1xx.h"

#define ENABLE_GPIO_CLOCK (RCC->APB2ENR |= RCC_APB2ENR_IOPCEN)

class STM32_GPIO : public GPIO
{
public:
    typedef enum {
        INPUT = 0x01, 
        OUTPUT = 0x02,
        PULL_UP = 0x04,
        PULL_DOWN = 0x08,
        OPEN_DRAIN = 0x10,
        ALT = 0x20,
    } IO;
private:
    static bool ck_enabled;

    GPIO_TypeDef *base;
    uint32_t mask;

    STM32_GPIO(GPIO_TypeDef *_base, uint32_t pin, IO io)
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

public:

    static GPIO* create(GPIO_TypeDef *_base, uint32_t _pin, IO io);
};

bool STM32_GPIO::ck_enabled = false;

GPIO *STM32_GPIO::create(GPIO_TypeDef *_base, uint32_t _pin, IO io)
{
    return new STM32_GPIO(_base, _pin, io);
}

#endif

    /*
     *
     */

void ms_delay(int ms)
{
   while (ms-- > 0) {
      volatile int x=500;
      while (x-- > 0)
         __asm("nop");
   }
}

#define LEDPORT (GPIOC)
#define LED1 (13)

int main(void)
{
    GPIO *gpio = STM32_GPIO::create(LEDPORT, LED1, STM32_GPIO::OUTPUT);

    while (true)
    {
        ms_delay(500);
        gpio->toggle();
    }
    return 0;
}
