#include "../RealTimeScheduler.h"

/**
 * Custom Task Control Block for handling Periodic Tasks
 */
typedef struct tskCustomTaskControlBlock{
    TaskFunction_t pxTaskCode;      // Pointer to task entry function
    TaskHandle_t *pxTaskHandle;     // Pointer to task handle
    BaseType_t xPriority;           // Priority at which the created task will execute
    BaseType_t xTaskJobStatus;      // Flag to check if the task is still running or not

    const char *pcName;             // Pointer to the descriptive name for the task
    UBaseType_t ulStackDepth;       // Number of words to allocate for use as the task's stack
    void *pvParameters;             // Pointer to optional arguments

    TickType_t xLastWakeTime;       // Parameter to track the last wake time of the task
    TickType_t xArrivalTime;        // Parameter for the arrival time of the task
    TickType_t xPeriod;             // Period of the task
    TickType_t xWCET;               // Worst-Case-Time-Execution
    TickType_t xWCRT;               // Worst-Case-Response-Time
    TickType_t xTimeSpent;          // Time elapsed since task start execution
    TickType_t xDeadline;           // Deadline of the task
    ListItem_t pxTCBItem;           // Item object for task list (owner)

    #if( configENABLE_APERIODIC == 1)
        BaseType_t xPS;             // Flag to know if the periodic task is a Polling Server
        TickType_t xBudgetPS;       // Budget of Polling Server
    #endif

    #if( configENABLE_EDF == 1 )
        TickType_t xAbsDeadline;    // Absolute deadline computed by the kernel
    #endif

}cTCB_t;

/**
 * Custom Task Control Block for handling Aperiodic Tasks
 */
#if( configENABLE_APERIODIC == 1 )
    typedef struct tskAperiodicTaskControlBlock{
        TaskFunction_t pxTaskCode;      // Pointer to task entry function
        const char *pcName;             // Pointer to the descriptive name for the task
        void *pvParameters;             // Pointer to optional arguments
        TickType_t xWCET;               // Worst-Case-Time-Execution
        ListItem_t pxTCBAItem;          // Item object for task list (owner)
        BaseType_t xArrival;            // Arrival Time used as key for sorting the queue
    } cTCBA_t;
#endif

/**
 * ---------------------------------------------------------------------------------
 * Variables by category
 */
static TickType_t xStartTime = 0;       // Counter time elapse since start
static BaseType_t xIdleFlag = 0;        // Flag to know if Idle Task is active
#if( configENABLE_RM == 1 )
    static List_t xTASK_List;           // List that contains all tasks TCBs
    static List_t *pxTASK_List = NULL;  // Pointer to task list initialization
#elif( configENABLE_EDF == 1 )
    static TaskHandle_t xSchedulerEDFHandle = NULL; // Task handle of task scheduler
    static List_t xTASK_List;                       // List that contains all tasks TCBs
    static List_t xTASK_List_tmp;                   // Temporaly List for removing items
    static List_t *pxTASK_List = NULL;              // Pointer to task list initialization
    static List_t *pxTASK_List_tmp = NULL;          // Pointer to temp task list initialization
#endif
#if(configENABLE_APERIODIC == 1 )
    static BaseType_t xArrival = -1;                // Arrival time used for sorting in queue
    static TaskHandle_t xPSHandle = NULL;           // Task handle of the Polling Server
    static List_t xAperiodicTASK_List;              // List that contains all aperiodic tasks TCBs
    static List_t *pxAperiodicTASK_List = NULL;     // Pointer to aperiodic task list initialization
#endif
/**
 * ---------------------------------------------------------------------------------
 */

/**
 * ---------------------------------------------------------------------------------
 * Declaration Functions by category
 */
static void prvCallTaskCreate();
static void prvPeriodicTaskMaster( void *pvParameters );
#if( configENABLE_RM == 1 )
    static void prvInitialiseTCBItemRMS( cTCB_t *pxTCB );
    static cTCB_t *prvGetTCBFromListByHandleRMS(TaskHandle_t xTaskHandle);
    #if(configENABLE_FIXED == 0)
        static void prvAssignPriorityRMS();
    #endif
#endif
#if( configENABLE_EDF == 1 )
    static void prvSetEDF();
    static void prvSwap( List_t **ppxList1, List_t **ppxList2 );
    static void prvCheckPrioritiesEDF();
    static void prvSchedulerEDFCode();
    static void prvSchedulerEDFCreate();
    static void prvInitialiseTCBItemEDF( cTCB_t *pxTCB );
    static void prvNotifySchedulerEDF();
    static cTCB_t *prvGetTCBFromListByHandleEDF(TaskHandle_t xTaskHandle);
#endif
#if( configENABLE_APERIODIC == 1 )
    static void prvInitialiseTCBAItem( cTCBA_t *pxTCBA );
    static void prvPollingServerCode();
    static void prvPollingServerInit();
#endif
#if( (configENABLE_RM == 1 || configENABLE_EDF == 1) && configENABLE_FIXED == 0 )
    static BaseType_t prvCheckFeasibilitySTD( void );
    static BaseType_t prvCheckFeasibilityWCRT( void );
#endif
/**
 * ---------------------------------------------------------------------------------
 */

/**
 * ---------------------------------------------------------------------------------
 * Library Functions
 */

/* Initialize Real Time environment */
void vInitScheduler(){
    #if(configENABLE_APERIODIC == 1)
        vListInitialise( &xAperiodicTASK_List );        // Creation of Tasks List
        pxAperiodicTASK_List = &xAperiodicTASK_List;    // Assigning pointer to List
    #endif
    #if( configENABLE_RM == 1)
        vListInitialise( &xTASK_List );     // Creation of Tasks List
        pxTASK_List = &xTASK_List;                // Assigning pointer to List
    #elif( configENABLE_EDF == 1 )
        vListInitialise( &xTASK_List );             // Creation of Tasks List
        vListInitialise( &xTASK_List_tmp );         // Creation of temp Tasks List
        pxTASK_List = &xTASK_List;                  // Assigning pointer to List
        pxTASK_List_tmp = &xTASK_List_tmp;          // Assigning pointer to temp List
    #endif
}

/* Function to create a custom TCB for periodic tasks and fill it with Task parameters set by user */
void vPeriodicTaskCreate( TaskFunction_t pxTaskCode, const char *pcName, TaskHandle_t *pxTaskHandle,
                          const uint32_t ulStackDepth, void *pvParameters, BaseType_t xPriority,
                          TickType_t xArrivalTime, TickType_t xPeriod, TickType_t xDeadline, TickType_t xWCET ){

    cTCB_t *pxTCB;
    pxTCB = pvPortMalloc(sizeof( cTCB_t ));
    if( pxTCB == NULL) return;

    pxTCB->pxTaskCode = pxTaskCode;
    pxTCB->pcName = pcName;
    pxTCB->pxTaskHandle = pxTaskHandle;
    pxTCB->ulStackDepth = ulStackDepth;
    pxTCB->pvParameters = pvParameters;
    pxTCB->xArrivalTime = xArrivalTime;
    pxTCB->xPriority = xPriority;
    pxTCB->xPeriod = xPeriod;
    pxTCB->xWCET = xWCET;
    pxTCB->xDeadline = xDeadline;
    pxTCB->xTaskJobStatus = pdTRUE;
    pxTCB->xTimeSpent = 0;
    pxTCB->xWCRT = xWCET;

    #if(configENABLE_APERIODIC == 1 )
        pxTCB->xPS = pdTRUE;
        pxTCB->xBudgetPS = configMAX_BUDGET_PS;
    #endif

    #if( configENABLE_EDF == 1 )
        pxTCB->xAbsDeadline = pxTCB->xDeadline + pxTCB->xArrivalTime + xStartTime;
        prvInitialiseTCBItemEDF( pxTCB );
    #endif

    #if( configENABLE_RM == 1)
        prvInitialiseTCBItemRMS( pxTCB );
    #endif

}

#if(configENABLE_APERIODIC == 1)
    /* Function to create a custom TCB for aperiodic tasks and fill it with Task parameters set by user */
    void vAperiodicTaskCreate( TaskFunction_t pxTaskCode, const char *pcName, void *pvParameters, TickType_t xWCET){
        cTCBA_t *pxTCBA = pvPortMalloc(sizeof( cTCBA_t ));
        xArrival++;
        pxTCBA->pxTaskCode = pxTaskCode;
        pxTCBA->pcName = pcName;
        pxTCBA->pvParameters = pvParameters;
        pxTCBA->xWCET = xWCET;
        pxTCBA->xArrival = xArrival;
        prvInitialiseTCBAItem( pxTCBA );

    }
#endif

/* Personalized vTaskStartScheduler function for Real Time environment */
void vTaskStartRealTimeScheduler(){
    #if(configENABLE_APERIODIC == 1)
        prvPollingServerInit();
    #endif
    #if( configENABLE_RM == 1 )
        #if( configENABLE_FIXED == 0)
            prvAssignPriorityRMS();
        #endif
    #elif( configENABLE_EDF == 1 )
        prvSetEDF();
        prvSchedulerEDFCreate();
    #endif

    #if( (configENABLE_RM == 1 || configENABLE_EDF == 1) && configENABLE_FIXED == 0 )
        if( prvCheckFeasibilitySTD() == pdFALSE )
            printf("\nSTANDARD FEASIBILITY TEST NOT PASSED\n");
        if( prvCheckFeasibilityWCRT() == pdFALSE ){
            printf( "\nFEASIBILITY WCRT TEST FAILED - The Task Set is not schedulable - EXITING PROGRAM\n\n");
            return;
        }else printf("\nFEASIBILITY WCRT TEST PASSED\n");
    #endif
    prvCallTaskCreate();
    xStartTime = xTaskGetTickCount();
    vTaskStartScheduler();
}
/**
 * ---------------------------------------------------------------------------------
 */

/**
 * ---------------------------------------------------------------------------------
 * Private Functions
 */

/*Hook function called in the System Tick Handler after any OS work is completed*/

void vApplicationTickHook( void ){
    #if( mainASSIGN_IP_DEMO == 1 )
        return;
    #endif
    TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();
    #if( configENABLE_RM == 1 )
        cTCB_t *pxTask = prvGetTCBFromListByHandleRMS(xCurrentTaskHandle);
        if( pxTask != NULL && xCurrentTaskHandle != xTaskGetIdleTaskHandle())
        {
            pxTask->xTimeSpent++;
            #if(configENABLE_APERIODIC == 1)
                if(pxTask->xPS == pdTRUE)
                    pxTask->xBudgetPS--;
            #endif
        }
    #elif( configENABLE_EDF == 1 )
        cTCB_t *pxTask = prvGetTCBFromListByHandleEDF(xCurrentTaskHandle);
        if( pxTask != NULL && xCurrentTaskHandle != xSchedulerEDFHandle && xCurrentTaskHandle != xTaskGetIdleTaskHandle())
            pxTask->xTimeSpent++;
    #endif
}

/* Hook function called when the Idle Task is activated*/
void vApplicationIdleHook( void ){
    xIdleFlag = 1;
}

#if( (configENABLE_RM == 1 || configENABLE_EDF == 1) && configENABLE_FIXED == 0 )
    /* Feasibility Test Standard*/
    static BaseType_t prvCheckFeasibilitySTD( void ){
        #if( configENABLE_RM == 1 )
            float xU = 1.0;
            float adder = 1.0;
        #elif( configENABLE_EDF == 1 )
            float xU = 0.0;
        #endif

        ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        cTCB_t *pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
            #if( configENABLE_RM == 1 )
                    xU *= (float) pxTCB->xWCET / pxTCB->xPeriod + adder ;
            #elif( configENABLE_EDF == 1 )
                    xU += (float) pxTCB->xWCET / pxTCB->xPeriod;
            #endif
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }

        #if( configENABLE_RM == 1 )
            if( xU > 2.0 ){
                printf("\nU_max > 2\n");
                return pdFALSE;
            }

        #elif( configENABLE_EDF == 1 )
            if( xU > 1.0 ){
                printf("\nU_max > 1\n");
                return pdFALSE;
            }
        #endif

        return pdTRUE;
    }

    /* Feasibility Test with Worst Case Response Time*/
    static BaseType_t prvCheckFeasibilityWCRT( void ){
        TickType_t xI;
        cTCB_t *pxTCB;

        ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        // Initialize R with WCET of the highest priority task
        TickType_t xR = pxTCB->xWCET;
        printf("\nTask %s - WCET %lu - WCRT %lu\n", pxTCB->pcName, pxTCB->xWCET, pxTCB->xWCRT);

        /*If the highest priority task has an execution time bigger than his period
         * the schedulability is already not feasible */
        if( pxTCB->xWCRT > pxTCB->xPeriod )
            return pdFALSE;

        // Take the next task
        pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );

            xR = xR + pxTCB->xWCET;
            // Do the test until R is not equal to the deadline (in RMS the deadline is equal to the period)
            while( xR <= pxTCB->xDeadline ){
                xI = 0;

                // Traverse the list and take the other task not equal to the task examined according to highest priority
                ListItem_t *pxTCB_HigherPriorityItem = listGET_HEAD_ENTRY( pxTASK_List );
                do{
                    cTCB_t *pxTCB_HigherPriority = listGET_LIST_ITEM_OWNER( pxTCB_HigherPriorityItem );
                    xI = xI + ( CEIL( xR, pxTCB_HigherPriority->xPeriod ) * pxTCB_HigherPriority->xWCET );
                    pxTCB_HigherPriorityItem = listGET_NEXT( pxTCB_HigherPriorityItem );
                }while( pxTCB_HigherPriorityItem != pxTCB_Pointer );

                xI = xI + pxTCB->xWCET;

                // If R is equal to I examine the next task
                if( xR == xI )
                    break;
                else
                    xR = xI;
            }

            if( xR > pxTCB->xDeadline )
                return pdFALSE;

            // Update WCRT
            pxTCB->xWCRT = xR;
            printf("Task %s - WCET %lu - WCRT %lu\n", pxTCB->pcName, pxTCB->xWCET, pxTCB->xWCRT);
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }

        return pdTRUE;
    }
#endif

/* Wrapper function calling all Task Code functions to execute them*/
static void prvPeriodicTaskMaster( void *pvParameters ){
    TaskHandle_t xCurrentTaskHandle = xTaskGetCurrentTaskHandle();
    #if( configENABLE_RM == 1 )
        cTCB_t *pxTask = prvGetTCBFromListByHandleRMS(xCurrentTaskHandle);
    #elif( configENABLE_EDF == 1 )
        cTCB_t *pxTask = prvGetTCBFromListByHandleEDF(xCurrentTaskHandle);
    #endif

    if ( pxTask->xArrivalTime != 0 )
        vTaskDelayUntil( &pxTask->xLastWakeTime, pxTask->xArrivalTime );
    if ( pxTask->xArrivalTime == 0 )
        pxTask->xLastWakeTime = xStartTime;

    for ( ; ; )
    {
        TickType_t xStartTick = xTaskGetTickCount();
        if( xIdleFlag == 1){
            printf("\n--------*[IDLE]*--------");
            xIdleFlag=0;
        }
        printf("\n------------------------------------------------------------------------------");
        #if( configENABLE_EDF == 1)
            prvNotifySchedulerEDF();
            printf( "\nTick Count %lu Task %s lastWakeTime %lu Abs deadline %lu Priority %ld\n",
                    xStartTick, pxTask->pcName, pxTask->xLastWakeTime,  pxTask->xAbsDeadline, pxTask->xPriority);
        #endif
        pxTask->xTaskJobStatus = pdFALSE;
        #if( configENABLE_RM == 1 )
            printf( "\n[START] Tick count %lu - Task %s - LastWakeTime %lu - Priority %ld \n", xStartTick, pxTask->pcName,
                    pxTask->xLastWakeTime, pxTask->xPriority );
        #endif
        pxTask->pxTaskCode( pvParameters );
        pxTask->xTaskJobStatus = pdTRUE;
        printf( "\n[END] Execution time %lu (WCET: %lu) - Task %s\r\n", pxTask->xTimeSpent, pxTask->xWCET, pxTask->pcName );
        pxTask->xTimeSpent = 0;

        #if( configENABLE_EDF == 1 )
            pxTask->xAbsDeadline = pxTask->xDeadline + pxTask->xLastWakeTime + pxTask->xPeriod;
            // Notify scheduler that the task has been executed and update priorities
            prvNotifySchedulerEDF();
        #endif

        vTaskDelayUntil( &pxTask->xLastWakeTime, pxTask->xPeriod );

    }
}

/* Create all tasks traversing the Task List*/
static void prvCallTaskCreate(){
    cTCB_t *pxTCB;

    ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
    const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

    while( pxTCB_Pointer != pxTCB_Tail ){
        pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
        xTaskCreate( prvPeriodicTaskMaster,
                     pxTCB->pcName,
                     pxTCB->ulStackDepth,
                     pxTCB->pvParameters,
                     pxTCB->xPriority,
                     pxTCB->pxTaskHandle);
        pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
    }
}
/**
 * ---------------------------------------------------------------------------------
 */

/**
 * ---------------------------------------------------------------------------------
 * Functions by Category
 */

#if( configENABLE_RM == 1 )
    /* Initialize TCB Item and insert it in Task List */
    static void prvInitialiseTCBItemRMS( cTCB_t *pxTCB ) {
        /* Initialize container to null so the item
         * does not think that it is already contained in a list*/
        vListInitialiseItem( &pxTCB->pxTCBItem );
        // The owner of a list item is the object (usually a TCB) that contains the list item
        listSET_LIST_ITEM_OWNER( &pxTCB->pxTCBItem, pxTCB );
        // The list is sorted in ascending order by period value
        listSET_LIST_ITEM_VALUE( &pxTCB->pxTCBItem, pxTCB->xPeriod);

        // Insert Item in Task List
        vListInsert( pxTASK_List, &pxTCB->pxTCBItem );
    }

    /* Return TCB from the Task List by Task Handle */
    static cTCB_t *prvGetTCBFromListByHandleRMS(TaskHandle_t xTaskHandle){
        cTCB_t *pxTCB;
        ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
            if ( strcmp(pxTCB->pcName, pcTaskGetName(xTaskHandle)) == 0 )
                return pxTCB;
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }
        return NULL;
    }

    #if(configENABLE_FIXED == 0)
        /* Assign a priori Task's priority traversing the Sorted List */
        static void prvAssignPriorityRMS() {
            cTCB_t *pxTCB;
            // Set priority of tasks starting by the max task priority - 1
            UBaseType_t xHighestPriority = configMAX_PRIORITIES - 1;

            ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
            const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

            while( pxTCB_Pointer != pxTCB_Tail ){
                pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
                // Set the priority from higher to lower since the list is sorted by ascending periods
                pxTCB->xPriority = xHighestPriority;
                xHighestPriority--;

                pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
            }
        }
    #endif
#endif

#if( configENABLE_EDF == 1 )
    /* Assign priority traversing the Sorted List */
    static void prvSetEDF(){
        cTCB_t *pxTCB;
        // Set priority of tasks starting by the scheduler priority - 1
        UBaseType_t xHighestPriority = configSCHED_PRIO - 1;

        ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
            // Set the priority from higher to lower since the list is sorted by ascending deadlines
            pxTCB->xPriority = xHighestPriority;
            xHighestPriority--;

            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }
    }

    /* Swap lists */
    static void prvSwap( List_t **ppxList1, List_t **ppxList2 ){
		List_t *pxList_tmp;
			pxList_tmp = *ppxList1;
			*ppxList1 = *ppxList2;
			*ppxList2 = pxList_tmp;
    }

    /* Assign new priorities to tasks based on their new absolute deadlines */
    static void prvCheckPrioritiesEDF(){
        cTCB_t *pxTCB;
        ListItem_t *pxTCB_Pointer;
        ListItem_t *pxTCB_Pointer_tmp;

        pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );
        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );

            // Sort again the list by the new task's deadlnine (updated absolute deadline)
            listSET_LIST_ITEM_VALUE( pxTCB_Pointer , pxTCB->xAbsDeadline );

            // Update list removing the previous on temp list and swapping
            pxTCB_Pointer_tmp = pxTCB_Pointer;
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
            uxListRemove( pxTCB_Pointer->pxPrevious );

            vListInsert( pxTASK_List_tmp, pxTCB_Pointer_tmp );
        }

        prvSwap( &pxTASK_List, &pxTASK_List_tmp);
        // Update highest priority
        BaseType_t xHighestPriority = configSCHED_PRIO - 1;

        pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
            pxTCB->xPriority = xHighestPriority;
            // Set the new updated priority to the target task
            vTaskPrioritySet( *pxTCB->pxTaskHandle, pxTCB->xPriority );

            xHighestPriority--;
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }
    }

    /* EDF Scheduler function code */
    static void prvSchedulerEDFCode(){
        for( ; ; ){
            prvCheckPrioritiesEDF();
            // Unblock target task
            ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        }
    }

    /* Create EDF Scheduler Task */
    static void prvSchedulerEDFCreate(){
        xTaskCreate( prvSchedulerEDFCode,
                     "SchedulerEDF",
                     configSCHED_MAX_STACK_DEPTH,
                     NULL,
                     configSCHED_PRIO,
                     &xSchedulerEDFHandle);
    }

    /* Return TCB from the Task List by Task Handle */
    static cTCB_t *prvGetTCBFromListByHandleEDF(TaskHandle_t xTaskHandle){
        cTCB_t *pxTCB;
        ListItem_t *pxTCB_Pointer = listGET_HEAD_ENTRY( pxTASK_List );
        const ListItem_t *pxTCB_Tail = listGET_END_MARKER( pxTASK_List );

        while( pxTCB_Pointer != pxTCB_Tail ){
            pxTCB = listGET_LIST_ITEM_OWNER( pxTCB_Pointer );
            if ( strcmp(pxTCB->pcName, pcTaskGetName(xTaskHandle)) == 0 )
                return pxTCB;
            pxTCB_Pointer = listGET_NEXT( pxTCB_Pointer );
        }
        return NULL;
    }

    /* Initialize TCB Item and insert it in Task List */
    static void prvInitialiseTCBItemEDF( cTCB_t *pxTCB ){
        vListInitialiseItem( &pxTCB->pxTCBItem );
        // The owner of a list item is the object (usually a TCB) that contains the list item
        listSET_LIST_ITEM_OWNER( &pxTCB->pxTCBItem, pxTCB );
        // The list is sorted in ascending order by deadline value
        listSET_LIST_ITEM_VALUE( &pxTCB->pxTCBItem, pxTCB->xAbsDeadline);

        // Insert in global list
        vListInsert( pxTASK_List, &pxTCB->pxTCBItem );
    }

    /* Call the EDF Scheduler Task */
    static void prvNotifySchedulerEDF(){
        BaseType_t xHigherPriorityTaskWoken;
        // Notify the scheduler task to execute again since an ISR has been activated during context witching to here
        vTaskNotifyGiveFromISR( xSchedulerEDFHandle, &xHigherPriorityTaskWoken );
        // Request a context switch to the higher priority task returned by the scheduler
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
#endif


#if( configENABLE_APERIODIC == 1 )
    /* Initialize TCB Item and insert it in Aperiodic Task List */
    static void prvInitialiseTCBAItem( cTCBA_t *pxTCBA ){
        vListInitialiseItem( &pxTCBA->pxTCBAItem );
        // The owner of a list item is the object (usually a TCB) that contains the list item
        listSET_LIST_ITEM_OWNER( &pxTCBA->pxTCBAItem, pxTCBA );
        // The list is sorted in ascending order by arrival time (FIFO)
        listSET_LIST_ITEM_VALUE( &pxTCBA->pxTCBAItem, pxTCBA->xArrival);

        // Insert in Aperiodic FIFO queue
        vListInsert( pxAperiodicTASK_List, &pxTCBA->pxTCBAItem );
    }

    /* Polling Server Code function that tries to execute Aperiodic Tasks */
    static void prvPollingServerCode(){
        for( ; ; ){
            if( pxAperiodicTASK_List->uxNumberOfItems == 0 ){
                printf("[PS] No Aperiodic Tasks to Serve\n");
                return;
            }
            else
            {
                cTCBA_t *pxTCBA;
                TaskHandle_t xTaskCurrentHandle = xTaskGetCurrentTaskHandle();

                // Get pointer to the TCB of the Polling Server
                cTCB_t *pxTCB = prvGetTCBFromListByHandleRMS(xTaskCurrentHandle);

                // Get the first aperiodic task from the Aperiodic FIFO queue
                ListItem_t *pxTCBA_Pointer = listGET_HEAD_ENTRY( pxAperiodicTASK_List );
                pxTCBA = listGET_LIST_ITEM_OWNER( pxTCBA_Pointer );

                // Check if the WCET of the aperiodic meets the budget of the polling server. If yes, execute it
                if(pxTCBA->xWCET < pxTCB->xBudgetPS){
                    pxTCBA->pxTaskCode( pxTCBA->pvParameters );
                    printf("\n[PS] Aperiodic Task %s executed - Polling Server Budget = %lu\n",
                           pxTCBA->pcName, pxTCB->xBudgetPS);
                }
                else    //Reset budget of Polling Server
                {
                    pxTCB->xBudgetPS = configMAX_BUDGET_PS;
                    printf("[PS] Next Aperiodic Task %s not schedulable - Reset Polling Server Budget to %lu\n",
                           pxTCBA->pcName, pxTCB->xBudgetPS);
                    return;
                }
                uxListRemove(pxTCBA_Pointer);
            }
        }
    }

    /* Create Polling Server (periodic) Task */
    static void prvPollingServerInit(){
        vPeriodicTaskCreate(prvPollingServerCode, "PS", &xPSHandle, configMINIMAL_STACK_SIZE, NULL,
                            8,pdMS_TO_TICKS(0),pdMS_TO_TICKS(900), pdMS_TO_TICKS(800), configMAX_BUDGET_PS);
    }
#endif
/**
 * ---------------------------------------------------------------------------------
 */
