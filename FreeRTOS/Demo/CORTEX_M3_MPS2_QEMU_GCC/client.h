#ifndef CORTEX_M3_MPS2_QEMU_GCC_CLIENT_H
#define CORTEX_M3_MPS2_QEMU_GCC_CLIENT_H

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>

/**
 * -------------------------------------------------------
 * Setting parameters
 * -------------------------------------------------------
 */

#define CLIENT_SEND_TASK_PRIORITY           ( tskIDLE_PRIORITY + 7 ) //needs to be higher than server priority
#define CLIENT_RECEIVE_TASK_PRIORITY        ( tskIDLE_PRIORITY + 5 ) //need to be lower than server priority
#define configDELAY                          4000

/**
 * -------------------------------------------------------
 * Library functions
 * -------------------------------------------------------
 */

void ReceiveClientIP(QueueHandle_t Queue, QueueHandle_t Queue2);
void Ping(uint32_t client_n, uint32_t priority);
void vClient_PING( void *pvParameters );
void vClient_FTP( void *pvParameters );
void vClient_WGET( void *pvParameters );

#endif
