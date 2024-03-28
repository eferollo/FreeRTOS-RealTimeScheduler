/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <FreeRTOS.h>
#include <task.h>

#include "FreeRTOSConfig.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <queue.h>
#include "server.h"
#include "client.h"
#include "RealTimeScheduler.h"

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName );
void vApplicationMallocFailedHook( void );

void vFullDemoIdleFunction( void );
void vFullDemoTickHookFunction( void );
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize );
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize );

extern void initialise_monitor_handles( void );
StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

TaskHandle_t xClient1_Handle = NULL;
TaskHandle_t xClient2_Handle = NULL;
TaskHandle_t xClient3_Handle = NULL;
TaskHandle_t xDNS = NULL;
TaskHandle_t xFirmware = NULL;

int main()
{
#if ( mainASSIGN_IP_DEMO == 1 )
    {
        AssignClientIP(); //start
        QueueHandle_t xQueueIP = getQueueIP();
        QueueHandle_t xQueueHostname = getQueueHostname();
        ReceiveClientIP(xQueueIP, xQueueHostname);
        Ping(2, 1);
        Ping(3, 2);
        Ping(4, 3); //first to be executed (higher priority)
        vTaskStartScheduler();
    }
#elif( mainFIXED_PRIORITY_NOPREEMPTION_DEMO == 1 )
    {
        /*--------------Activate Fixed Priority--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 0
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 1
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,7, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(400), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(600), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 9,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(200));
        //unfeasable
        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#elif ( mainFIXED_PRIORITY_PREEMPTION_DEMO == 1 )
    {
        /*--------------Activate Fixed Priority with Preemption--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 1
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,9, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(400), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(600), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 7,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(200));

        vTaskStartRealTimeScheduler();
        //This is the worst case with higher priority to FTP and lower priority to Ping
        for( ;  ; )
        {
        }
    }
#elif( mainROUND_ROBIN_DEMO == 1 )
    {
        /*--------------Activate Round Robin--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 1
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 1
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,8, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(400), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(600), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(200));

        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#elif ( mainRM_NOAPERIODIC_DEMO == 1 )
    {
        /*--------------Activate RMS--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 0
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,7, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(300), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(700), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 9,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(600), pdMS_TO_TICKS(200));

        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#elif (main_RM_DIFF_ARRIVAL_DEMO == 1)
    {
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,7, pdMS_TO_TICKS(700),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(400), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(800),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(600), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 9,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(200));

        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#elif( mainEDF_NOAPERIODIC_DEMO == 1 )
    {
        /*--------------Activate EDF--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_EDF 1
            #define configENABLE_FIXED 0
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,9, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(200), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(600), pdMS_TO_TICKS(200));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(300));
        //UNFEASABLE FOR RMS
        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#elif ( mainRM_WCRT_DEMO == 1 )
    {
        /*--------------Activate RMS--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 0
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,7, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(300), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(400), pdMS_TO_TICKS(400), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 9,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(400), pdMS_TO_TICKS(400), pdMS_TO_TICKS(100));

        vTaskStartRealTimeScheduler();
        //In this case the standard feasability check is not passed but the WCRT yes
        for( ;  ; )
        {
        }
    }
#elif ( mainRM_APERIODIC_DEMO == 1 )
    {
        /*--------------Activate RMS and Polling Server--------------
        In FreeRTOSConfig.h:
            #define configUSE_PREEMPTION  = 1
            #define configUSE_TIME_SLICING  = 0
        In RealTimeScheduler.h:
            #define configENABLE_RM 1
            #define configENABLE_FIXED 0
            #define configENABLE_APERIODIC 1
        */
        vInitScheduler();
        vPeriodicTaskCreate(vClient_PING, "Client1", &xClient1_Handle, configMINIMAL_STACK_SIZE, NULL,7, pdMS_TO_TICKS(0),
                                pdMS_TO_TICKS(300), pdMS_TO_TICKS(200), pdMS_TO_TICKS(50));
        vPeriodicTaskCreate(vClient_WGET, "Client2", &xClient2_Handle, configMINIMAL_STACK_SIZE, NULL, 8,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(700), pdMS_TO_TICKS(600), pdMS_TO_TICKS(150));
        vPeriodicTaskCreate(vClient_FTP, "Client3", &xClient3_Handle, configMINIMAL_STACK_SIZE, NULL, 9,pdMS_TO_TICKS(0),
                            pdMS_TO_TICKS(600), pdMS_TO_TICKS(400), pdMS_TO_TICKS(200));

        vAperiodicTaskCreate(DNS, "Server", &xDNS, pdMS_TO_TICKS(24));
        vAperiodicTaskCreate(DNS, "Server", &xDNS, pdMS_TO_TICKS(24));
        vAperiodicTaskCreate(Firmware, "Server", &xFirmware, pdMS_TO_TICKS(40));
        vAperiodicTaskCreate(Firmware, "Server", &xFirmware, pdMS_TO_TICKS(40));

        vTaskStartRealTimeScheduler();

        for( ;  ; )
        {
        }
    }
#endif
    return 0;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
     * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
     * internally by FreeRTOS API functions that create tasks, queues, software
     * timers, and semaphores.  The size of the FreeRTOS heap is set by the
     * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Run time stack overflow checking is performed if
     * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     * function is called if a stack overflow is detected. */
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

void vAssertCalled( void )
{
    volatile unsigned long looping = 0;

    taskENTER_CRITICAL();
    {
        /* Use the debugger to set ul to a non-zero value in order to step out
         *      of this function to determine why it was called. */
        while( looping == 0LU )
        {
            portNOP();
        }
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/
void vLoggingPrintf( const char * pcFormat,
                     ... )
{
    va_list arg;

    va_start( arg, pcFormat );
    vprintf( pcFormat, arg );
    va_end( arg );
}

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
     * state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
    /* If the buffers to be provided to the Timer task are declared inside this
     * function then they must be declared static - otherwise they will be allocated on
     * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

