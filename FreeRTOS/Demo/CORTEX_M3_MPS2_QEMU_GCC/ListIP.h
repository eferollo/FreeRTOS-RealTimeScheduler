#ifndef LISTIP_H
#define LISTIP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>

/*
 * Structure to contain a "table" of known client IPs and corresponding hostnames
 */
typedef struct ipList_s *ipList_t;
ipList_t ipList_init();

void ipList_insert(ipList_t ipList, char *IP, char *hostname);
void ipList_free(ipList_t ipList);
void ipList_check(ipList_t ipList, char *IP);

#endif
