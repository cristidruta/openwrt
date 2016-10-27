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


#define PCTL_MSG_ADDR "/var/pctl_msg_addr"
#define MAX_PCTL_ID_LEN 16
#define MAX_PCTL_MSG_LEN 512

typedef enum {
    PCTL_MSG_REGISTER=0,
    PCTL_MSG_DEV_ONLINE,
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

int g_pCtlFd;


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

static int PCtlMsg_receive(int fd, PCtlMsgHeader **buf)
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

    *buf = msg;

    return 0;
}

int PCtlMsg_send(int fd, const PCtlMsgHeader *buf)
{
   unsigned int totalLen;
   int rc;
   int ret=0;

   totalLen = sizeof(PCtlMsgHeader) + buf->dataLength;

   rc = write(fd, buf, totalLen);
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

void readMsgFromPCtl(void)
{
    PCtlMsgHeader *msg=NULL;
    int ret=-1;

    while ((ret=PCtlMsg_receive(g_pCtlFd, &msg)) == 0)
    {
        switch(msg->type)
        {
            case PCTL_MSG_DEV_ONLINE:
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

int pCtlIntf_init()
{
    int ret=-1;
    if ((ret=pCtlSock_init()) < 0)
    {
        printf("pCtlSock_init failed!\r\n");
    }
    return ret;
}

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
            printf("error on select, errno=%d\r\n", errno);
            usleep(100);
            continue;
        }

        if (FD_ISSET(g_pCtlFd, &rset)) {
            readMsgFromPCtl();
        }
    }

    return 0;
}
