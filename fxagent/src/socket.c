#include "share.h"
#include "parser.h"

#define TCP_KEEPALIVE 1 // use TCP keepalive feature

struct sockaddr_in server_addr;

int is_socket_connected(int fd);
int socket_init(void);
int socket_create(void);
int socket_connect(void);

int socket_init(void)
{
    struct timeval timeout = {3,0}; 
    /* This func is used to listen the event which cloud server send to me(After register succeed).*/
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);

    if ( 0 == socket_create())
    {
        socket_connect();
    }

    //setsockopt(cloudc_monitor_uloop_fd.fd, SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

    if(1 == TCP_KEEPALIVE)
    {
        int keepalive = 1;      // use TCP keepalive feature
        int keepidle = 30;     
        int keepinterval = 10;  
        int keepcount = 3;     

        int res = setsockopt(cloudc_monitor_uloop_fd.fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));
        if (res < 0)
        {
            perror("setsockopt error:");
        }

        int res1 = setsockopt(cloudc_monitor_uloop_fd.fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle , sizeof(keepidle));
        if (res1 < 0)
        {
            perror("setsockopt error:");
        }


        int res2 = setsockopt(cloudc_monitor_uloop_fd.fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval));
        if (res2 < 0)
        {
            perror("setsockopt error:");
        }

        int res3 = setsockopt(cloudc_monitor_uloop_fd.fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount));
        if (res3 < 0)
        {
            perror("setsockopt error:");
        }
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}



int socket_create(void)
{
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);
    if ((cloudc_monitor_uloop_fd.fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket error:");
        //sleep(10);
        return -1;
    }

    cloudc_debug("%s[%d]: socket crreate succeed, fd = %d", __func__, __LINE__, cloudc_monitor_uloop_fd.fd);
    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

int socket_connect(void)
{

    int count = 0;
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(cloudc_server_ip);
    server_addr.sin_port        = htons(cloudc_server_port);

    while (1)
    {
        int ret = connect(cloudc_monitor_uloop_fd.fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0)
        {
            perror("connect error:");
            sleep(1);
            //count = count + 1;
            //sleep(10*count);
        }
        else
        {
            cloudc_debug("%s[%d]: socket connect succeed, ret = %d", __func__, __LINE__, ret);
            cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
            return 0;
        }
    }
}


/* to check if the socket is still up: */
int is_socket_connected(fd)
{
    int error = 0;
    socklen_t len = sizeof (error);

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    int ret = getsockopt (fd, SOL_SOCKET, SO_ERROR, &error, &len);

    if (ret != 0)  
    {   
        cloudc_debug("failed to get socket error code: %s", strerror(ret));
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return -1; 
    }   

    if (error != 0)  
    {   
        cloudc_error("socket error: %s", strerror(errno));
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return -1; 
    }   

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

