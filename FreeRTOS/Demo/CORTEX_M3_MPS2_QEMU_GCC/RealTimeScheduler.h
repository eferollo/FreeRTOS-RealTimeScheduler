#ifndef REALTIMESCHEDULER_H
#define REALTIMESCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/**
 * -------------------------------------------------------
 * Setting parameters
 * -------------------------------------------------------
 */

#define configSCHED_PRIO (configMAX_PRIORITIES - 1)
#define configSCHED_MAX_STACK_DEPTH 2000
#define configMAX_BUDGET_PS pdMS_TO_TICKS(50)
#define CEIL( x, y )    (( x / y ) + ( x % y != 0 ))

/**
 * -------------------------------------------------------
 * Types of RTS algorithms
 *
 * NOTE:
 *      1. To enable fixed priority, RMS MUST be enabled too
 *      2. To enable aperiodic tasks, RMS MUST be enabled too
 *      3. To enable EDF scheduler, all others MUST be disabled
 * -------------------------------------------------------
 */

#define configENABLE_EDF        0
#define configENABLE_RM         1
#define configENABLE_FIXED      0
#define configENABLE_APERIODIC  0

/**
 * -------------------------------------------------------
 * Library functions
 * -------------------------------------------------------
 */

void vInitScheduler();
void vPeriodicTaskCreate( TaskFunction_t pxTaskCode, const char *pcName, TaskHandle_t *pxTaskHandle,
                          const uint32_t ulStackDepth, void *pvParameters, BaseType_t xPriority,
                          TickType_t xArrivalTime, TickType_t xPeriod, TickType_t xDeadline, TickType_t xWCET);
void vAperiodicTaskCreate( TaskFunction_t pxTaskCode, const char *pcName, void *pvParameters, TickType_t xWCET);
void vTaskStartRealTimeScheduler();

#endif
