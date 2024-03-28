#include "ListIP.h"

typedef struct node *link;

struct node{
    char *IP;
    char *hostname;
    link next;
};

struct ipList_s{
    link head;
    link tail;
    int nIP;
};

link newNode(char *IP, char *hostname, link next){
    link x = pvPortMalloc(sizeof *x);
    if(x == NULL)
        return NULL;
    x->IP = IP;
    x->hostname = hostname;
    x->next = next;
    return x;
}

ipList_t ipList_init(){
    ipList_t ipList = pvPortMalloc(sizeof(*ipList));
    ipList->head = NULL;
    ipList->tail = NULL;
    ipList->nIP = 0;
    return ipList;
}

void ipList_insert(ipList_t ipList, char *IP, char *hostname){
    link x = newNode(IP, hostname, NULL);
    if(ipList->head == NULL){
        ipList->tail = x;
        ipList->head = ipList->tail;
    }else{
        ipList->tail->next = x;
        ipList->tail = x;
    }
    ipList->nIP++;
}

void ipList_free(ipList_t ipList){
    link x,p;
    for(x = ipList->head; x != NULL; x = p){
        p=x->next;
        vPortFree(x->hostname);
        vPortFree(x->IP);
        vPortFree(x);
    }
    vPortFree(ipList);
}

/*
 * This function takes in an ipList_t structure (the table containing the clients known to the server) and a string (the incoming client's IP).
 * It then iterates through the list of IP addresses and compares each IP address with the given IP.
 * If a match is found, it prints a message indicating that a ping was received from the corresponding hostname found.
 */
void ipList_check(ipList_t ipList, char *IP){
    link x;
    for(x= ipList-> head; x!=NULL ; x=x->next){
        if(strcmp(x->IP,IP)==0)
            printf("Ping received from %s (IP: %s)", x->hostname, IP);
    }
    printf("Ping received from unknown host");
}