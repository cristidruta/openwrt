#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include "msg.h"

extern void sendMsg_devOnline();
extern void message_handle(int fd);

#define MAX_IP_LEN 16
#define MAX_PORT_LEN 6
char server[MAX_IP_LEN]="127.0.0.1";
char port[MAX_PORT_LEN]="9999";

static void get_config(int argc, char *argv[])
{
    char tmpStr[128]={0};
    char *tmpIp=NULL, *tmpPort=NULL;

    if (argc < 1)
    {
        printf("usage: ipdevc ip:port\r\n");
        return;
    }

    strncpy(tmpStr, argv[1], sizeof(tmpStr) - 1 );
    tmpIp = strtok(tmpStr,":");
    tmpPort = strtok(NULL, ":");

    if((NULL != tmpIp) && (NULL != tmpPort))
    {
        strncpy(server, tmpIp, MAX_IP_LEN-1);
        strncpy(port, tmpPort, MAX_PORT_LEN-1);
    }

    return;
}

void main(int argc, char *argv[])
{
    int fd=-1, maxFd=-1;
    int ret=-1;
    fd_set readfds;

    get_config(argc, argv);

    if((fd=devMsg_initSocket(server, port)) < 0)
    {
        printf("devMsg_initSocket() failed\r\n");
        return;
    }

    sendMsg_devOnline();

    maxFd = fd;

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        ret = select(maxFd+1, &readfds, NULL, NULL, NULL);
        if (ret==-1)
        {
            continue;
        }
        message_handle(fd);
    }

    close(fd);
}
