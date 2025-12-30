
#include <assert.h>

    #include "stm32f1xx.h"
    #define LEDPORT (GPIOC)
    #define LED1 (13)
    #define ENABLE_GPIO_CLOCK (RCC->APB2ENR |= RCC_APB2ENR_IOPCEN)
    #define _MODER    CRH
    #define GPIOMODER (GPIO_CRH_MODE13_0)

#include "panglos/debug.h"

//#include "panglos/drivers/uart.h"
#include "panglos/drivers/gpio.h"

#if defined(ASSERT)
#undef ASSERT
#define ASSERT(x) assert(x)
#endif

using namespace panglos;

class STM32_GPIO : public GPIO
{
public:
    typedef enum { INPUT, OUTPUT } IO;
private:
    static bool ck_enabled;

    GPIO_TypeDef *base;
    uint32_t pin;

    STM32_GPIO(GPIO_TypeDef *_base, uint32_t _pin, IO io)
    :   base(_base),
        pin(_pin)
    {
        if (!ck_enabled)
        {
            ENABLE_GPIO_CLOCK;
            ck_enabled = true;
        }
        ASSERT(io == OUTPUT); // TODO
        base->_MODER |= GPIOMODER;   // set pins to be general purpose output
    }

    virtual void set(bool state) override
    {
        if (state)
            base->ODR |= (1 << pin);
        else
            base->ODR &= ~(1 << pin);
    }
    virtual bool get() override
    {
        return base->ODR & (1 << pin);
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
