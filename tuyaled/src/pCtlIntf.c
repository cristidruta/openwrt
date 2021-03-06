#include "pCtlIntf.h"


#define PCTL_MSG_ADDR "/var/pctl_msg_addr"
#define PCTL_CLIENT_ID "tuya_led"


int g_pCtlFd=-1;
static unsigned int g_seq=1;


int pCtlSock_init()
{
    struct sockaddr_un serverAddr;
    int rc;
    int fd=-1;

    /* Create a unix domain socket. */
    if ((fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket!\r\n");
        return -1;
    }


    /*
     * Set close-on-exec, even though all apps should close their
     * fd's before fork and exec.
     */
    if ((rc = fcntl(fd, F_SETFD, FD_CLOEXEC)) != 0)
    {
        printf("set close-on-exec failed, rc=%d errno=%d\r\n", rc, errno);
        close(fd);
        return -1;
    }


    /* Connect to server. */
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, PCTL_MSG_ADDR, sizeof(serverAddr.sun_path));

    rc = connect(fd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (rc != 0)
    {
        printf("connect to %s failed, rc=%d errno=%d\r\n", PCTL_MSG_ADDR, rc, errno);
        close(fd);
        return -1;
    }

    g_pCtlFd = fd;
    return 0;
}

int PCtlMsg_receive(int fd, PCtlMsgHeader **buf)
{
    PCtlMsgHeader *msg;
    char *inBuf;
    int rc;

    if (buf == NULL)
    {
        printf("buf is NULL!");
        return -4;
    }
    else
    {
        *buf = NULL;
    }

    /*
     * Read just the header in the first read.
     * Do not try to read more because we might get part of 
     * another message in the TCP socket.
     */
    msg = (PCtlMsgHeader *) malloc(sizeof(PCtlMsgHeader) + MAX_PCTL_MSG_LEN);
    if (msg == NULL)
    {
        printf("alloc of msg header failed");
        return -2;
    }

    rc = read(fd, msg, sizeof(PCtlMsgHeader));
    if ((rc == 0) || ((rc == -1) && (errno == 131)))
    {
        /* broken connection */
        free(msg);
        return -3;
    }
    else if (rc < 0 || rc != sizeof(PCtlMsgHeader))
    {
        printf("bad read, rc=%d errno=%d", rc, errno);
        free(msg);
        return -1;
    }

    if (msg->dataLength > 0)
    {
        if (msg->dataLength > MAX_PCTL_MSG_LEN)
        {
            msg = (PCtlMsgHeader *)realloc(msg, sizeof(PCtlMsgHeader) + msg->dataLength - MAX_PCTL_MSG_LEN);
            if (msg == NULL)
            {
                printf("realloc to %d bytes failed", sizeof(PCtlMsgHeader) + msg->dataLength);
                free(msg);
                return -2;
            }
        }

        /* there is additional data in the message */
        inBuf = (char *) (msg + 1);
        rc = read(fd, inBuf, msg->dataLength);
        if (rc <= 0)
        {
            printf("bad data read, rc=%d errno=%d", rc, errno);
            free(msg);
            return -1;
        }
        else if (rc < msg->dataLength) {
            printf("bad data read, rc=%d expected=%d", rc, msg->dataLength);
            free(msg);
            return -1;
        }
    }

    *buf = msg;

    return 0;
}

int PCtlMsg_send(PCtlMsgHeader *buf)
{
   unsigned int totalLen;
   int rc;
   int ret=0;

   strcpy(buf->pCtlId, PCTL_CLIENT_ID);
   buf->seq=g_seq++;

   totalLen = sizeof(PCtlMsgHeader) + buf->dataLength;

   rc = write(g_pCtlFd, buf, totalLen);
   if (rc < 0)
   {
      if (errno == EPIPE)
      {
         printf("got EPIPE, dest app is dead\r\n");
         return -2;
      }
      else
      {
         printf("write failed, errno=%d\r\n", errno);
         ret = -1;
      }
   }
   else if (rc != (int) totalLen)
   {
      printf("unexpected rc %d, expected %u\r\n", rc, totalLen);
      ret = -1;
   }

   return ret;
}

int pCtlIntf_init()
{
    int ret=-1;
    PCtlMsgHeader msg;

    if ((ret=pCtlSock_init()) < 0)
    {
        printf("pCtlSock_init failed!\r\n");
    }

    memset(&msg, 0, sizeof(PCtlMsgHeader));
    msg.type = PCTL_MSG_REGISTER;

    if ((ret=PCtlMsg_send(&msg)) < 0) {
        pctl_error("PCtlMsg_send() failed! ret=%d\r\n", ret);
    }

    return ret;
}

#if 0
void readMsgFromPCtl(void)
{
    PCtlMsgHeader *msg=NULL;
    int ret=-1;

    while ((ret=PCtlMsg_receive(g_pCtlFd, &msg)) == 0)
    {
        switch(msg->type)
        {
            case PCTL_MSG_DEV_ONLINE:
                devOnlineHandle();
                break;

            default:
                printf("unrecognized msg 0x%x\r\n", msg->type);
                break;
        }

        free(msg);
        msg=NULL;
    }

    return;
}
#endif

#if 0
int pCtlIntf_loop()
{
    int max_fd=g_pCtlFd;
    int nready;
    int ret=0;
    fd_set rset;

    /* detach from the terminal so we don't catch the user typing control-c.
    if (setsid() == -1)
    {
        printf("Could not detach from terminal");
    } */

    for (;;) {
        FD_ZERO(&rset);
        FD_SET(g_pCtlFd,&rset);

        nready = select(max_fd+1, &rset, NULL, NULL, NULL);
        if (nready == -1)
        {
            pctl_error("error on select, errno=%d\r\n", errno);
            usleep(100);
            continue;
        }

        if (FD_ISSET(g_pCtlFd, &rset)) {
            readMsgFromPCtl();
        }
    }

    return 0;
}
#endif
