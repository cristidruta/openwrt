#ifndef __PCTLINTF_H__
#define __PCTLINTF_H__
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <fcntl.h>
#include "log.h"

#define pctl_debug adapt_debug
#define pctl_error adapt_error

#define MAX_PCTL_ID_LEN 16
#define MAX_PCTL_MSG_LEN 512

typedef enum {
    PCTL_MSG_REGISTER=0,
    PCTL_MSG_DEV_ONLINE,
    PCTL_MSG_DEV_SET,
} PCtlMsgType;

typedef struct pctl_msg_header
{
   unsigned char pCtlId[MAX_PCTL_ID_LEN];
   PCtlMsgType  type;
   unsigned int seq;
   struct pctl_msg_header *next;
   unsigned int wordData;
   int dataLength;
} PCtlMsgHeader;
#endif
