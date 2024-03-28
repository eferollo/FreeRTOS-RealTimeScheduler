#ifndef CORTEX_M3_MPS2_QEMU_GCC_SERVER_H
#define CORTEX_M3_MPS2_QEMU_GCC_SERVER_H

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include <string.h>
#include <ListIP.h>

/**
 * -------------------------------------------------------
 * Setting parameters
 * -------------------------------------------------------
 */

#define SERVER_TASK_PRIORITY                ( tskIDLE_PRIORITY + 6 )
#define mainQUEUE_LENGTH                    ( 5 )

/**
 * -------------------------------------------------------
 * Library functions
 * -------------------------------------------------------
 */

void AssignClientIP();
QueueHandle_t getQueueIP();
QueueHandle_t getQueuePing();
QueueHandle_t getQueueHostname();
void Firmware( void *pvParameters );
void DNS( void *pvParameters );

#endif

