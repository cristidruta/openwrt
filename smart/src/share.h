#ifndef _SHARE_H_
#define _SHARE_H_


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>     /* for AF_NETLINK */
#include <sys/ioctl.h>      /* for ioctl */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/netlink.h>  /* for sockaddr_nl */
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <libubox/ustream.h>
#include <libubus.h>
#include <signal.h>
#include <libubox/blobmsg_json.h>
#include <uci.h>
#include "logSmart.h"

#define SMART_SERVER_MANAGE_IP "192.168.1.10"
#define SMART_SERVER_MANAGE_PORT 3000
#define RECV_MAX_BUF_LEN 1024
#define SEND_MAX_BUF_LEN 1024
#define RECV_TIME_OUT 30
#define SEND_TIME_OUT 30

#define IEEEADDR_LEN 8

//extern struct uloop_fd smartUloopFd;
extern struct uloop_fd ipUloopFd;
extern struct uloop_fd zigbeeUloopFd;
extern struct uloop_fd bluetoothUloopFd;
extern struct uloop_fd configFd;

extern struct sockaddr_in server_addr;
//extern struct ubus_context *smart_ctx;

extern int smart_log_level_int;


#endif
