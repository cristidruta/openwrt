#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include "msg.h"

extern void sendMsg_devOnline();
extern void message_handle(int fd);

void main()
{
    int fd=-1, maxFd=-1;
    int ret=-1;
    fd_set readfds;
    char *server="127.0.0.1";
    char *port="9999";

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
