#include "iotc.h"
#include "cJSON.h"

extern int iotc_handleServerPkt(char *recvbuf);

static int iotc_readDataFromSocket(int fd, char *buf)
{
    int len=-1;

    iotc_debug("Enter.");

    len=recv(fd, buf, RECV_MAX_BUF_LEN, 0);

    if ((len <= 0))
    {
        iotc_error("recv from fd(%d) error: %s, len = %d", fd, strerror(errno), len);
    }

    iotc_debug("Exit.");
    return len;
}

static void iotc_recvServerData(struct uloop_fd *u, unsigned int events)
{
    int len=0;
    char buf[RECV_MAX_BUF_LEN] = {0};
    
    iotc_debug("Enter.");

    len=iotc_readDataFromSocket(u->fd, buf);

    iotc_debug("len=%d recv_buf=%s", len, buf);

    if (len <= 0)
    {
        //iotc_error("recieve error! %s, len=%d", strerror(errno), len);
    }
    else
    {
        iotc_handleServerPkt(buf);
    }

    iotc_debug("Exit.");
}

struct uloop_fd iotc_monitor_uloop_fd = {
    .cb = iotc_recvServerData,
    .fd = -1
};
