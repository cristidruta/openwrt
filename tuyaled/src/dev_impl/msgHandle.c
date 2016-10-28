#include "datatypes.h"
#include "msg.h"

void handle_devOnline(client_t *c, DevMsgHeader *msg)
{
    char buf[sizeof(DevMsgHeader)+sizeof(MsgRsp_DevOnline)]={0};
    DevMsgHeader *rsp=(DevMsgHeader *)buf;
    MsgRsp_DevOnline *info=(MsgRsp_DevOnline *)(rsp+1);

    rsp->type = DEV_MSG_DEV_ONLINE;
    rsp->flag |= MSG_FLAG_BIT_RSP;
    rsp->dataLength = sizeof(MsgRsp_DevOnline);
    
    info->clientId = 12345;
    strcpy(info->manufacture, "tuya");
    strcpy(info->manufactureSN, "00112233445566");
 
    if (devMsg_send(c, rsp) != 0)
    {
        printf("send DEV_ONLINE rsp msg failed!\r\n");
    }
    else
    {
        printf("send DEV_ONLINE rsp msg succeed!\r\n");
    }
}

void devMsg_handle(client_t *c, DevMsgHeader *msg)
{
    switch(msg->type) 
    {
        case DEV_MSG_DEV_ONLINE:
            handle_devOnline(c, msg);
            break;

        default:
            printf("unknown msg %d\r\n", msg->type);
            break;
    }
}
