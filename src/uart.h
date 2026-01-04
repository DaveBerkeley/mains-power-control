
#include "panglos/drivers/uart.h"

#if defined(STM32F1)

#include "stm32f1xx.h"

class STM32_UART : public panglos::UART
{
public:
    static panglos::UART *create(USART_TypeDef *_base, uint32_t rx_size);
};

#endif  //  STM32F1

//  FIN
