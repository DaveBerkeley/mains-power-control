
#include "panglos/drivers/gpio.h"

#if defined(STM32F1)

#if defined(STM32F103xB)
    // blue-pill board!
#endif

#include "stm32f1xx.h"

class STM32_GPIO : public panglos::GPIO
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

    static panglos::GPIO* create(GPIO_TypeDef *_base, uint32_t _pin, IO io);
};

#endif  //  STM32F1
