#include "logger.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>

static SemaphoreHandle_t loggerMutex = NULL;

void loggerInit(void)
{
    if (loggerMutex == NULL)
    {
        loggerMutex = xSemaphoreCreateMutex();
    }
}

void loggerPrint(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    BaseType_t schedulerRunning = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);

    if (schedulerRunning && (loggerMutex != NULL))
    {
        if (xSemaphoreTake(loggerMutex, portMAX_DELAY) == pdTRUE)
        {
            vprintf(fmt, args);
            xSemaphoreGive(loggerMutex);
        }
        else
        {
            /* Se nao conseguir pegar o mutex, ainda assim tenta logar para nao perder info. */
            vprintf(fmt, args);
        }
    }
    else
    {
        /* Antes do scheduler iniciar ou sem mutex, apenas delega direto. */
        vprintf(fmt, args);
    }

    va_end(args);
}
