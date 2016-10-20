#include "iotc.h"
#include <netinet/in.h>
#include <netinet/tcp.h>

struct sockaddr_in server_addr;

extern struct uloop_fd iotc_monitor_uloop_fd;
extern char iotc_server_ip[];
extern int iotc_server_port; 

static int socket_create(void)
{
    iotc_debug("Enter.");

    if ((iotc_monitor_uloop_fd.fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        iotc_error("failed to socket! %s", strerror(errno));
        return -1;
    }

    iotc_debug("socket create succeed, fd=%d", iotc_monitor_uloop_fd.fd);

    iotc_debug("Exit.");
    return 0;
}

static int socket_connect(void)
{
    iotc_debug("Enter.");

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(iotc_server_ip);
    server_addr.sin_port        = htons(iotc_server_port);

    while (1)
    {
        if (connect(iotc_monitor_uloop_fd.fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0)
        {
            iotc_debug("connect to server succeed.");
            return 0;
        }

        sleep(5);
        iotc_debug("trying to connect to server...");
    }
}

int connection_init(void)
{
    int res=-1;
    int keepalive=1;      // 开启keepalive属性
    int keepidle=30;     // 如该连接在30秒内没有任何数据往来,则进行探测
    int keepinterval=10;  // 探测时发包的时间间隔为10秒
    int keepcount=3;      // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    int fd=-1;

    iotc_debug("Enter.");

    if ( 0 != socket_create())
    {
        iotc_debug("socket_create failed!");
        return -1;
    }

    socket_connect();

    fd=iotc_monitor_uloop_fd.fd;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive)) < 0)
    {
        iotc_error("setsockopt SO_KEEPALIVE failed! %s", strerror(errno));
    }

    if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle)) < 0)
    {
        iotc_error("setsockopt TCP_KEEPIDLE failed! %s", strerror(errno));
    }

    if (setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval, sizeof(keepinterval)) < 0)
    {
        iotc_error("setsockopt TCP_KEEPINTVL failed! %s", strerror(errno));
    }

    if (setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount, sizeof(keepcount)) < 0)
    {
        iotc_error("setsockopt TCP_KEEPCNT failed! %s", strerror(errno));
    }

    iotc_debug("Exit.");
    return 0;
}


/* to check if the socket is still up: */
int is_socket_connected(fd)
{
    int error=0;
    socklen_t len=sizeof(error);
    int ret=0;

    iotc_debug("Enter.");

    ret=getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (ret != 0)  
    {
        iotc_debug("failed to get socket error code: %s", strerror(ret));
        return -1;
    }

    if (error != 0)  
    {
        iotc_error("socket error: %s", strerror(errno));
        return -1;
    }

    iotc_debug("Exit.");
    return 0;
}
