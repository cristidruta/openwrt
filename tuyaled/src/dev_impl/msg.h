#ifndef __DEV_MSG_H__
#define __DEV_MSG_H__

int devMsg_initServerSocket(unsigned int port);

int devMsg_receive(int fd, DevMsgHeader **buf);

int devMsg_send(client_t *c, DevMsgHeader *buf);
#endif
