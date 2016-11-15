#include "datatypes.h"
#include "msg.h"
#include "../log.h"
//#include "../share.h"
//#define MAX_MANUFACTURESN_LEN 32
//#define MAX_MANUFACTURE_LEN 32


extern int devOnlineHandle();
extern "C" void getDevInfoByDevId(char *deviceId, char *manufactureSN, char *manufacture);
//extern int devOnlineHandle();
void handle_devOnline(client_t *c, DevMsgHeader *msg)
{
    char buf[sizeof(DevMsgHeader)+sizeof(MsgRsp_DevOnline)]={0};
    DevMsgHeader *rsp=(DevMsgHeader *)buf;
    MsgRsp_DevOnline *info=(MsgRsp_DevOnline *)(rsp+1);


  //  Msg_DevOnline *InFo=(Msg_DevOnline *)(msg+1);
    c->manufactureSN = "002000345ccf7f1b7e15";
    c->manufacture =    "tuya";

    rsp->type = DEV_MSG_DEV_ONLINE;
    rsp->flag |= MSG_FLAG_BIT_RSP;
    rsp->dataLength = sizeof(MsgRsp_DevOnline);
    
    info->clientId = 12345;
    strcpy(info->manufacture, "tuya");
    strcpy(info->manufactureSN, "00112233445566");
    //devOnlineHandle();
    if (devMsg_send(c, rsp) != 0)
    {
        adapt_debug("send DEV_ONLINE rsp msg failed!\r\n");
    }
    else
    {
        adapt_debug("send DEV_ONLINE rsp msg succeed!\r\n");
    }
    if (devOnlineHandle() != 0)
    {
        adapt_debug("send DEV_ONLINE msg to agent failed!\r\n");
    }
    else 
    {
        adapt_debug("send DEV_ONLINE msg to agent succeed!\r\n");
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
            adapt_debug("unknown msg %d\r\n", msg->type);
            break;
    }
}

extern client_t *devMgmt_getClient(char *manufactureSN, char *manufacture);

void send_fwUpgradeMsg(char *url, char *deviceid)
{
    char buf[sizeof(DevMsgHeader)+sizeof(Msg_FwUpgrade)] = {0};
    DevMsgHeader *msg = (DevMsgHeader *)buf;
    Msg_FwUpgrade *info = (Msg_FwUpgrade *)(msg+1);
    client_t *c=NULL;
    
    char ManufactureSN[MAX_MANUFACTURESN_LEN] = {0};
    char Manufacture[MAX_MANUFACTURE_LEN] = {0}; 
    getDevInfoByDevId(deviceid, ManufactureSN, Manufacture);
    adapt_debug("ManufactureSN = %s,Manufacture = %s", ManufactureSN, Manufacture);
    c = devMgmt_getClient(ManufactureSN, Manufacture);

    if (c != NULL )
    {
        msg->type = DEV_MSG_FWUPGRADE;
        msg->dataLength =sizeof(Msg_FwUpgrade);

        //info->clientId = 12345;
        strcpy(info->url, url);
        if (devMsg_send(c, msg) != 0)
        {
            adapt_debug("send DEV_MSG_FWUPGRADE msg failed!\r\n");
        }
        else
        {
            adapt_debug("send DEV_MSG_FWUPGRADE msg succeed!\r\n");
        }
    }

    adapt_debug("EXIT: quit the send_fwUpgradeMsg!");
    return;
}
void send_configMsg(char *json_buf, char *Manufacture, char *ManufactureSN)
{
    char buf[sizeof(DevMsgHeader)+sizeof(Msg_FwUpgrade)] = {0};
    DevMsgHeader *msg = (DevMsgHeader *)buf;
    Msg_Config *info = (Msg_Config*)(msg+1);
    client_t *c = NULL;
    c = devMgmt_getClient(ManufactureSN, Manufacture);

    if (c != NULL)
    {
        msg->type = DEV_MSG_CONFIG;
        msg->dataLength = sizeof(Msg_Config);

        strcpy(info->sendbuf, json_buf);
        if (devMsg_send(c, msg) != 0)
        {
            adapt_debug("send DEV_MSG_CONFIG msg failed!\r\n");
        }
        else
        {
            adapt_debug("send DEV_MSG_CONFIG msg succeed!\r\n");
        }
    }
    adapt_debug("EXIT: quit the send_configMsg!\r\n");
    return;
}


