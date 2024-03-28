#include "client.h"

static void SendTask(void *pvParameters);
static void ReceiveTask(void *pvParameters);
static void PingTask(void *pvParameters);

static QueueHandle_t xQueueIP = NULL;
static QueueHandle_t xQueueHostname = NULL;
static uint32_t cnt_s = 0UL;
static uint32_t cnt_r = 0UL;
static char clientList[5][10] = {"client1", "client2", "client3", "client4", "client5"};
TaskHandle_t xReceiveHandle = NULL;
TaskHandle_t xSendHandle = NULL;
TaskHandle_t xPingHandle = NULL;


typedef struct{
    char *cIP;
    char *cHostname;
}client_t;

client_t *clientConf = NULL;

void vClient_PING( void *pvParameters ){
    (void ) pvParameters;
    int i, out;
    printf("\n");
    for( i = 0; i < 7000000 ; i++ )
    {
        out = 1 + i * i * i * i * i * i;
        out = out / 8;
    }
    printf("\n[CLIENT] Client 1 pings: %d Bytes", 64 );
}


void vClient_WGET( void *pvParameters ){
    (void ) pvParameters;
    int i,out;
    printf("\n");
    for( i = 0; i < 11000000 ; i++ )
    {
        out = 1 + i * i * i * i * i * i * i;
        out = out*(1 + i * i * i * i * i * i * i);
        if( i % 1100000 == 0 && out != 0 && i>0){
            printf("\n[CLIENT] Page status: %d0%%", (i/1100000));
        }
    }
    printf("\n[CLIENT] Request completed!");
}

void vClient_FTP( void *pvParameters ){
    (void ) pvParameters;
    int i,out;
    printf("\n");
    for( i = 0; i < 24000000 ; i++ )
    {
        out = 1 + i * i * i * i * i * i * i * i;
        out = out/(1 + i * i * i * i * i * i * i * i);
        if( i % 2400000 == 0 && out != 0 && i>0){
            printf("\n[CLIENT] File Transfer Status: %d0%%", (i/2400000));
        }
    }
    printf("\n[CLIENT] File transfer completed!");
}

void ReceiveClientIP(QueueHandle_t Queue, QueueHandle_t Queue2){

    xQueueIP = Queue;
    xQueueHostname = Queue2;
    clientConf = pvPortMalloc(5*sizeof(client_t));
    uint32_t i;

    for(i=0UL; i<5UL; i++){
        xTaskCreate( SendTask,
                     "SendClientTask",
                     configMINIMAL_STACK_SIZE,
                     (void *)clientConf,
                     CLIENT_SEND_TASK_PRIORITY,
                     &xSendHandle);

        xTaskCreate( ReceiveTask,
                     "ReceiveClientTask",
                     configMINIMAL_STACK_SIZE,
                     (void *)clientConf,
                     CLIENT_RECEIVE_TASK_PRIORITY,
                     &xReceiveHandle);
    }
}

void Ping(uint32_t client_n, uint32_t priority){

   uint32_t PING_TASK_PRIORITY = ( tskIDLE_PRIORITY + priority );

            xTaskCreate( PingTask,
                         "PingTask",
                         configMINIMAL_STACK_SIZE,
                         (void *) client_n,
                         PING_TASK_PRIORITY,
                         &xPingHandle);
}

static void PingTask(void *pvParameters)
    {
        TickType_t xNextWakeTime;

        /* Remove compiler warning about unused parameter. */
        uint32_t client_n = (uint32_t) pvParameters - 1; //offset to adjust ip

        /* Initialise xNextWakeTime - this only needs to be done once. */
        xNextWakeTime = xTaskGetTickCount();

        for( ; ; )
        {
            printf("Ping sent from %s\n", clientConf[client_n].cIP);
            /* Place this task to let others to ping */
            vTaskDelayUntil( &xNextWakeTime, configDELAY);
        }
    }


static void SendTask(void *pvParameters){

    client_t *clientConf = (client_t *)pvParameters;

    clientConf[cnt_s].cHostname = clientList[cnt_s];
    printf("[CLIENT] Sending IP request configuration by %s\n", clientConf[cnt_s].cHostname);
    xQueueSend( xQueueHostname, &clientConf[cnt_s].cHostname, 10U );
    cnt_s++;

    vTaskDelete(NULL);
}

static void ReceiveTask(void *pvParameters){

    char *cReceivedIP;
    client_t *clientConf = (client_t *)pvParameters;

    while(uxQueueMessagesWaiting(xQueueIP) != 0){
        xQueueReceive(xQueueIP, &cReceivedIP, portMAX_DELAY);
        clientConf[cnt_r].cIP = cReceivedIP;
        cnt_r++;
        printf("[CLIENT] IP %s received and configured\n", cReceivedIP);
        vTaskDelete(NULL);
    }

}

