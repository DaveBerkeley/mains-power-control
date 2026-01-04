
#include <stdint.h>

void Timers_Init(void);
void TIM3_SetPulseWidth(uint32_t pulse_width);
void TIM4_SetPulseWidth(uint32_t pulse_width);

extern volatile uint32_t period_us;
extern volatile uint8_t capture_ready;

//  FIN
