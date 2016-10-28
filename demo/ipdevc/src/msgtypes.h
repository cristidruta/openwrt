#ifndef __DEV_MSGTYPES_H__
#define __DEV_MSGTYPES_H__
typedef enum {
    DEV_MSG_DEV_ONLINE=0,
} DevMsgType;

typedef struct dev_msg_header
{
   DevMsgType  type;
   unsigned int clientId;
   unsigned int seq;
   unsigned int wordData;
   int dataLength;
} DevMsgHeader;

int devMsg_initSocket(char *srvIp, char *port);
int devMsg_receive(int fd, DevMsgHeader **buf);
int devMsg_send(int fd, DevMsgHeader *buf);
#endif
