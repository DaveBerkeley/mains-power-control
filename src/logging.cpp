
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/logger.h"

using namespace panglos;

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

extern volatile uint32_t tick; // TODO : put in a suitable header

uint32_t get_time(void) { return tick; }
const char *get_task(void) { return ""; }

void Error_Handler(void)
{
    while (true) 
        ;
}

extern "C" bool arch_in_irq()
{
    // TODO
    return false;
}

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
