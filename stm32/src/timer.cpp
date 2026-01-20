
#include "timer.h"

//#if defined(STM32F1)

#include "stm32f1xx.h"


    /*
     *
     */

// Timer configuration
// TIM2 - Input Capture on CH1 (PA0)
// TIM3 - Calibration pulse on CH1 (PA6)
// TIM4 - MOC3020 trigger on CH1 (PB6)

volatile uint32_t period_us = 0;
volatile uint8_t capture_ready = 0;

// Initialize TIM2 for input capture with trigger output and auto-reset
void TIM2_InputCapture_Init(void)
{
    // Enable clocks
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Timer configuration
    TIM2->PSC = 7;  // (8MHz / 8) = 1MHz
    TIM2->ARR = 0xFFFFFFFF;  // Max period (32-bit timer)
    
    // Configure CC1 as input, IC1 mapped to TI1
    TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;
    
    // No input filter, no prescaler
    TIM2->CCMR1 &= ~(TIM_CCMR1_IC1F | TIM_CCMR1_IC1PSC);
    
    // Capture on rising edge
    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    
    // Enable capture and interrupt
    TIM2->CCER |= TIM_CCER_CC1E;
    TIM2->DIER |= TIM_DIER_CC1IE;
    
    // SLAVE MODE: Reset mode - counter resets on TI1 rising edge
    TIM2->SMCR &= ~TIM_SMCR_TS;
    TIM2->SMCR |= (TIM_SMCR_TS_2 | TIM_SMCR_TS_0);  // TS = 101 = TI1FP1
    TIM2->SMCR &= ~TIM_SMCR_SMS;
    TIM2->SMCR |= TIM_SMCR_SMS_2;  // SMS = 100 = Reset mode
    
    // MASTER MODE: Generate trigger on Reset (when counter resets)
    TIM2->CR2 &= ~TIM_CR2_MMS;
    TIM2->CR2 |= TIM_CR2_MMS_1;  // MMS = 010 = Update
    
    // Enable TIM2 interrupt in NVIC
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 0);
    
    // Start timer
    TIM2->CR1 |= TIM_CR1_CEN;
}

// Initialize TIM3 for calibration pulse with hardware trigger
void TIM3_OneShot_Init(void)
{
    // Enable clocks
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    
    // Timer configuration
    TIM3->PSC = 7;       // 8MHz / 8 = 1MHz (1µs per tick)
    TIM3->CCR1 = 1;      // Delay: 1µs
    TIM3->ARR = 500;     // Total period: 501µs
    
    // One-pulse mode
    TIM3->CR1 = TIM_CR1_OPM;
    
    // MASTER MODE: Generate trigger on update event for TIM4
    TIM3->CR2 = TIM_CR2_MMS_1;  // MMS = 010 = Update event
    (void)TIM3->CR2;  // Ensure write completes
    
    // PWM mode 2: Output LOW when CNT < CCR1, HIGH when CNT >= CCR1
    TIM3->CCMR1 = (7 << 4) | TIM_CCMR1_OC1PE;
    
    // Enable output
    TIM3->CCER = TIM_CCER_CC1E;
    
    // Load registers
    TIM3->EGR = TIM_EGR_UG;
    
    // SLAVE MODE: Triggered by TIM2 (ITR1)
    TIM3->SMCR = (1 << 4) | (6 << 0);  // TS=001, SMS=110
}

// Initialize TIM4 for MOC3020 trigger pulse on PB6
void TIM4_TriggerPulse_Init(void)
{
    // Enable clocks
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    
    // Timer configuration
    TIM4->PSC = 7;      // 8MHz / 8 = 1MHz
    TIM4->CCR1 = 1;     // Pulse starts immediately
    TIM4->ARR = 20;     // 20µs pulse width
    
    // One-pulse mode
    TIM4->CR1 = TIM_CR1_OPM;
    
    // PWM mode 2
    TIM4->CCMR1 = (7 << 4) | TIM_CCMR1_OC1PE;
    
    // Enable output
    TIM4->CCER = TIM_CCER_CC1E;
    
    // Load registers
    TIM4->EGR = TIM_EGR_UG;
    
    // SLAVE MODE: Triggered by TIM3 (ITR2)
    TIM4->SMCR = (2 << 4) | (6 << 0);  // TS=010 (ITR2=TIM3), SMS=110
}

// TIM2 interrupt handler - period is directly available in CCR1
extern "C" void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_CC1IF)
    {
        TIM2->SR &= ~TIM_SR_CC1IF;
        period_us = TIM2->CCR1;
        capture_ready = 1;
    }
}

// Initialize all timers
void Timers_Init(void)
{
    TIM2_InputCapture_Init();
    TIM3_OneShot_Init();
    TIM4_TriggerPulse_Init();
}

// Set TIM3 calibration pulse width (in microseconds)
void TIM3_SetPulseWidth(uint32_t pulse_width_us)
{
    TIM3->ARR = pulse_width_us;
}

// Set TIM4 trigger pulse width (optional, default 20µs)
void TIM4_SetPulseWidth(uint32_t pulse_width_us)
{
    TIM4->ARR = pulse_width_us;
    if (!pulse_width_us)
    {
        // Disable timer
        TIM4->CCER &= ~TIM_CCER_CC1E;
    }
    else
    {
        // Enable timer
        TIM4->CCER = TIM_CCER_CC1E;
    }
}

//  FIN
