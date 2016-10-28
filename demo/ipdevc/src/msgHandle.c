#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "msg.h"

void sendMsg_devOnline()
{
    char buf[sizeof(DevMsgHeader)+sizeof(Msg_DevOnline)]={0};
    DevMsgHeader *msg=(DevMsgHeader *)buf;
    Msg_DevOnline *info=(Msg_DevOnline *)(msg+1);

    msg->type = DEV_MSG_DEV_ONLINE;
    strcpy(info->manufacture, "tuya");
    strcpy(info->manufactureSN, "00112233445566");
 
    if (devMsg_send(msg) != 0)
    {
        printf("send DEV_ONLINE msg failed!\r\n");
    }
    else
    {
        printf("send DEV_ONLINE msg succeed!\r\n");
    }
}

void message_handle(int fd)
{
    DevMsgHeader *msg=NULL;

    while (devMsg_receive(fd, &msg) == 0)
    {
        switch(msg->type)
        {
            case DEV_MSG_DEV_ONLINE:
                // it's a response
                if (msg->flag & MSG_FLAG_BIT_RSP)
                {
                    MsgRsp_DevOnline *info=(MsgRsp_DevOnline *)(msg+1);
                    devMsp_setClientId(info->clientId);
                    printf("get DEV_ONLINE rsp msg succeed! clientId=%d\r\n", info->clientId);
                }
                break;

            case DEV_MSG_FWUPGRADE:
                {
                    Msg_FwUpgrade *info=(Msg_FwUpgrade *)(msg+1);
                    printf("firmware upgrade: %s-%s\r\n", info->version, info->url);
                }
                break;

            default:
                printf("unknown msg %d\r\n", msg->type);
        }

        free(msg);
    }
}
