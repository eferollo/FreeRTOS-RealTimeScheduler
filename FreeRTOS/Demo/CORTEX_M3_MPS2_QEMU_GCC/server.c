#include "server.h"

static QueueHandle_t xQueuePing = NULL;
static QueueHandle_t xQueueIP = NULL;
static QueueHandle_t xQueueHostname = NULL;
static uint32_t counter = 0UL;

TaskHandle_t xServerHandle = NULL;
static void AssignTask(void *pvParameters);

QueueHandle_t getQueuePing(){
    return xQueuePing;
}

QueueHandle_t getQueueHostname(){
    return xQueueHostname;
}

QueueHandle_t getQueueIP(){
    return xQueueIP;
}

char *generateIP(){
    char *cAddress = pvPortMalloc(16*sizeof (char));
    snprintf(cAddress, 16, "10.0.1.%ld", counter);
    counter++;

    return cAddress;
}

void AssignClientIP(){
    xQueueIP = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );
    xQueueHostname = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );

    ipList_t ipList = ipList_init();
    char *cIP = generateIP();
    ipList_insert(ipList, cIP, "server");

    if(xQueueIP != NULL){
        xTaskCreate( AssignTask,
                     "ServerAssign",
                     configMINIMAL_STACK_SIZE,
                     (void *)ipList,
                     SERVER_TASK_PRIORITY,
                     &xServerHandle );
    }

}

static void AssignTask(void *pvParameters){

    char *cReceivedHostname;
    ipList_t ipList = (ipList_t )pvParameters;

    for( ; ; )
    {
        while(uxQueueMessagesWaiting(xQueueHostname) != 0){

            xQueueReceive( xQueueHostname, &cReceivedHostname, portMAX_DELAY );
            printf("[SERVER] %s is asking for an IP\n", cReceivedHostname);
            char *cIP = generateIP();
            ipList_insert(ipList, cIP, cReceivedHostname);
            xQueueSend(xQueueIP, &cIP, 0U);
            printf("[SERVER] IP %s assigned to %s host\n", cIP, cReceivedHostname);
        }
        vTaskDelete(NULL);
    }

}

void DNS( void *pvParameters ){
    (void) pvParameters;
    int i;
    int out;
    printf("\n");
    for( i = 0; i < 2500000 ; i++ )
    {
        out = 1 + i * i * i * i * i * i;
        out = out / 8;
    }
    printf("\n[SERVER] DNS Update");
}

void Firmware( void *pvParameters ){
    (void) pvParameters;
    int i;
    int out;
    printf("\n");
    for( i = 0; i < 7000000 ; i++ )
    {
        out = 1 + i * i * i * i * i * i;
        out = out / 8;
    }
    printf("\n[SERVER] Firmware Update");
}


