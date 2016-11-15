#ifndef __DEV_MSG_H__
#define __DEV_MSG_H__
typedef enum {
    DEV_MSG_DEV_ONLINE=0,
    DEV_MSG_SET,
    DEV_MSG_FWUPGRADE,
    DEV_MSG_CONFIG
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
#define MAX_MANUFACTURE_LEN 32
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
#define MAX_SENDBUF_LEN 1024
typedef struct{
    char sendbuf[MAX_SENDBUF_LEN];
}Msg_Config;

/* fixed part, do not change */
int devMsg_initServerSocket(unsigned int port);
int devMsg_receive(int fd, DevMsgHeader **buf);
int devMsg_send(client_t *c, DevMsgHeader *buf);
#endif
