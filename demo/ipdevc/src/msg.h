#ifndef __DEV_MSG_H__
#define __DEV_MSG_H__

typedef enum {
    DEV_MSG_DEV_ONLINE=0,
    DEV_MSG_SET,
    DEV_MSG_FWUPGRADE
} DevMsgType;

#define MSG_FLAG_BIT_RSP 1
typedef struct dev_msg_header
{
   DevMsgType  type;
   unsigned int clientId;
   unsigned int flag;
   unsigned int seq;
   unsigned int wordData;
   int dataLength;
} DevMsgHeader;

/* MSG: DEV_ONLINE */
#define MAX_MANUFACTURE_LEN 16
#define MAX_MANUFACTURESN_LEN 64
typedef struct {
    char manufacture[MAX_MANUFACTURE_LEN];
    char manufactureSN[MAX_MANUFACTURESN_LEN];
} Msg_DevOnline;

typedef struct {
    unsigned int clientId;
    char manufacture[MAX_MANUFACTURE_LEN];
    char manufactureSN[MAX_MANUFACTURESN_LEN];
} MsgRsp_DevOnline;

/* MSG: DEV_FWUPGRADE */
#define MAX_VERSION_LEN 16
#define MAX_URL_LEN 256
typedef struct {
    char version[MAX_VERSION_LEN];
    char url[MAX_URL_LEN];
} Msg_FwUpgrade;

/* fix part, do not change */
int devMsg_initSocket(char *srvIp, char *port);
int devMsg_receive(int fd, DevMsgHeader **buf);
int devMsg_send(DevMsgHeader *buf);
void devMsp_setClientId(unsigned clientId);
#endif
