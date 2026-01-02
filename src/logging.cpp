
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/logger.h"

using namespace panglos;

extern volatile uint32_t tick; // TODO : put in a suitable header

#if defined(STM32F1xx) || defined(STM32F4xx)

#define NVIC_ICSR   (*((volatile uint32_t *) 0xe000ed04))
#define IS_IN_IRQ() ((NVIC_ICSR & 0x1FFUL) != 0)

extern "C" bool arch_in_irq()
{
    return IS_IN_IRQ();
}

#endif  //  STM32F1xx

    /*
     *  Logging interface
     */

const LUT Severity_lut[] = {
    // Logging Severity
    {   "NONE", S_NONE, },
    {   "CRITICAL", S_CRITICAL, },
    {   "ERROR", S_ERROR, },
    {   "WARNING", S_WARNING, },
    {   "NOTICE", S_NOTICE, },
    {   "INFO", S_INFO, },
    {   "DEBUG", S_DEBUG, },
    { 0, 0 },
};

uint32_t get_time(void) { return tick; }
const char *get_task(void) { return "M"; }

void Error_Handler(void)
{
    PO_ERROR("");
    while (true) 
        ;
}

    /*
     *
     */

extern "C" void po_log(Severity s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (logger)
    {
        logger->log(s, fmt, ap);
    }
    else
    {
        vprintf(fmt, ap);
    }
    va_end(ap);
}

//  FIN
