
#include "timer.h"

//#if defined(STM32F1)

#include "stm32f1xx.h"


    /*
     *
     */

// Timer configuration
// TIM2 - Input Capture on CH1 (PA0)
// TIM3 - One-shot output on CH1 (PA6)

volatile uint32_t period_us = 0;
volatile uint8_t capture_ready = 0;

// Initialize TIM2 for input capture with trigger output and auto-reset
void TIM2_InputCapture_Init(void)
{
    // Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    
    // Configure PA0 as input floating (TIM2_CH1)
    GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOA->CRL |= GPIO_CRL_CNF0_0;  // Floating input
    
    // Timer configuration
    // Prescaler: divide by 8 to get 1MHz (1us resolution) assuming 8MHz clock
    TIM2->PSC = 7;  // (8MHz / 8) = 1MHz
    TIM2->ARR = 0xFFFFFFFF;  // Max period (32-bit timer)
    
    // Configure CC1 as input, IC1 mapped to TI1
    TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;
    TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;  // CC1 channel is input, IC1 mapped to TI1
    
    // No input filter, no prescaler
    TIM2->CCMR1 &= ~(TIM_CCMR1_IC1F | TIM_CCMR1_IC1PSC);
    
    // Capture on rising edge (default, but explicit)
    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    
    // Enable capture and interrupt
    TIM2->CCER |= TIM_CCER_CC1E;  // Enable capture
    TIM2->DIER |= TIM_DIER_CC1IE; // Enable CC1 interrupt
    
    // SLAVE MODE: Reset mode - counter resets on TI1 rising edge
    TIM2->SMCR &= ~TIM_SMCR_TS;     // Clear trigger selection
    TIM2->SMCR |= (TIM_SMCR_TS_2 | TIM_SMCR_TS_0);  // TS = 101 = Filtered Timer Input 1 (TI1FP1)
    
    TIM2->SMCR &= ~TIM_SMCR_SMS;    // Clear slave mode
    TIM2->SMCR |= TIM_SMCR_SMS_2;   // SMS = 100 = Reset mode
    
    // MASTER MODE: Generate trigger on Reset (when counter resets)
    TIM2->CR2 &= ~TIM_CR2_MMS;
    TIM2->CR2 |= TIM_CR2_MMS_1;  // MMS = 010 = Update (counter reset generates TRGO)
    
    // Enable TIM2 interrupt in NVIC (for period measurement)
    NVIC_EnableIRQ(TIM2_IRQn);
    NVIC_SetPriority(TIM2_IRQn, 0);
    
    // Start timer
    TIM2->CR1 |= TIM_CR1_CEN;
}

// Initialize TIM3 for one-shot output with hardware trigger
void TIM3_OneShot_Init(void)
{
    // Enable clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    
    // Configure PA6 as alternate function push-pull (TIM3_CH1)
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
    GPIOA->CRL |= GPIO_CRL_CNF6_1 | GPIO_CRL_MODE6;  // AF push-pull, 50MHz
    
    // Timer configuration - set prescaler FIRST
    TIM3->PSC = 7;  // Same 1us resolution (8MHz / 8)
    TIM3->ARR = 1000;  // Total period: 1000us
    TIM3->CCR1 = 1000;  // Pulse width
    
    // Generate update event to load PSC, ARR and CCR1 into shadow registers
    TIM3->EGR = TIM_EGR_UG;
    // Clear the UIF flag that gets set by UG
    TIM3->SR = 0;
    
    // WORKAROUND: Manually enable and disable timer to load prescaler
    // The prescaler only loads on an actual counter update event with CEN=1
    TIM3->CR1 |= TIM_CR1_CEN;  // Enable counter briefly
    TIM3->CR1 &= ~TIM_CR1_CEN; // Disable it
    TIM3->CNT = 0;             // Reset counter
    TIM3->SR = 0;              // Clear all flags again
    
    // One-pulse mode - MUST be set BEFORE configuring output
    TIM3->CR1 |= TIM_CR1_OPM;  // One-pulse mode
    TIM3->CR1 |= TIM_CR1_ARPE; // Auto-reload preload enable
    
    // Configure CC1 as output
    TIM3->CCMR1 = 0;  // Clear first
    TIM3->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);  // PWM mode 1 (110)
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE;  // Output compare preload enable
    
    // Enable output - active high
    TIM3->CCER = TIM_CCER_CC1E;
    
    // HARDWARE TRIGGER CONFIGURATION - configure LAST
    TIM3->SMCR = 0;  // Clear first
    TIM3->SMCR |= TIM_SMCR_TS_0;    // TS = 001 = ITR1 (TIM2 for TIM3)
    TIM3->SMCR |= (TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1);  // SMS = 110 = Trigger mode
    
    // Don't start TIM3 manually - it will be triggered by TIM2
}

// Trigger one-shot pulse (optional - only needed if you want to change pulse width dynamically)
void TIM3_SetPulseWidth(uint32_t pulse_width_us)
{
    // Update pulse width (will take effect on next trigger)
    TIM3->ARR = pulse_width_us;
    TIM3->CCR1 = pulse_width_us;
}

// TIM2 interrupt handler - period is directly available in CCR1!
extern "C" void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_CC1IF)
    {
        // Clear interrupt flag
        TIM2->SR &= ~TIM_SR_CC1IF;
        
        // Read captured value - this IS the period!
        // Counter resets to 0 on each capture, so CCR1 contains the period directly
        period_us = TIM2->CCR1;
        capture_ready = 1;
        
        // TIM3 is triggered automatically by hardware - no need to call TIM3_TriggerPulse()!
    }
}

// Initialize both timers
void Timers_Init(void)
{
    TIM2_InputCapture_Init();  // Input capture first
    TIM3_OneShot_Init();       // Then one-shot output
}

