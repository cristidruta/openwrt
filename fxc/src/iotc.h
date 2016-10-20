#ifndef __IOTC_H__
#define __IOTC_H__

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <libubox/uloop.h>
#include "cJSON.h"
#include "iotc_log.h"

#define MAX_IP_LEN 16
#define SEND_MAX_BUF_LEN 4096
#define RECV_MAX_BUF_LEN 4096

typedef enum {
    MSG_DEV_ONLINE_RSP=0,
    MSG_SET,
    MSG_UNKNOWN
}AgentMsgType;

extern struct uloop_fd iotc_monitor_uloop_fd;

/* from glue.c */
int glue_init();
char* glue_getDevData();
int glue_isMyself(char *manufacture, char *manufactureSN);
void glue_updateDevId(char *devId);

/* from proto.c */
void iotc_genHostStr(char *server);
int iotc_sendDevOnlineMsg();
int iotc_sendSetRspMsg(int cmdId, int retCode, cJSON *jsonDevData);

/* from session.c */
int iotcSess_gotRsp(AgentMsgType msgType, unsigned int cmdId);
int iotcSess_waitRsp(AgentMsgType msgType, unsigned int cmdId);
#endif
