#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "msg.h"

#define MAX_DEV_MSG_LEN 512

static unsigned int g_seq=1;
static unsigned int g_clientId=0;
static int g_fd=-1;

void devMsp_setClientId(unsigned clientId)
{
    g_clientId = clientId;
}

int devMsg_initSocket(char *srvIp, char *port)
{
    struct sockaddr_in server;
    int sock=-1, rc;

    if ((sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket");
        return -1;
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(srvIp);
    server.sin_port = htons(atoi(port));

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
        printf("connect to %s:%s failed, err=%s\r\n", srvIp, port, strerror(errno));
        return -1;
    }

    printf("msg socket is ready (fd=%d)\r\n", sock);

    g_fd = sock;

    return sock;
}

int devMsg_receive(int fd, DevMsgHeader **buf)
{
    DevMsgHeader *msg;
    char *inBuf;
    int rc;

    if (buf == NULL)
    {
        printf("buf is NULL!\r\n");
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
    msg = (DevMsgHeader *) malloc(sizeof(DevMsgHeader) + MAX_DEV_MSG_LEN);
    if (msg == NULL)
    {
        printf("alloc of msg header failed\r\n");
        return -2;
    }

    //rc = read(fd, msg, sizeof(DevMsgHeader));
    rc = recv(fd, msg, sizeof(DevMsgHeader), 0);
    if ((rc == 0) || ((rc == -1) && (errno == 131)))
    {
        /* broken connection */
        free(msg);
        return -3;
    }
    else if (rc < 0 || rc != sizeof(DevMsgHeader))
    {
        printf("bad read, rc=%d errno=%d\r\n", rc, errno);
        free(msg);
        return -1;
    }

    if (msg->dataLength > 0)
    {
        if (msg->dataLength > MAX_DEV_MSG_LEN)
        {
            msg = (DevMsgHeader *)realloc(msg, sizeof(DevMsgHeader) + msg->dataLength - MAX_DEV_MSG_LEN);
            if (msg == NULL)
            {
                printf("realloc to %d bytes failed\r\n", sizeof(DevMsgHeader) + msg->dataLength);
                free(msg);
                return -2;
            }
        }

        /* there is additional data in the message */
        inBuf = (char *) (msg + 1);
        //rc = read(fd, inBuf, msg->dataLength);
        rc = recv(fd, inBuf, msg->dataLength, 0);
        if (rc <= 0)
        {
            printf("bad data read, rc=%d errno=%d\r\n", rc, errno);
            free(msg);
            return -1;
        }
        else if (rc < msg->dataLength) {
            printf("bad data read, rc=%d expected=%d\r\n", rc, msg->dataLength);
            free(msg);
            return -1;
        }
    }

    *buf = msg;

    return 0;
}

int devMsg_send(DevMsgHeader *buf)
{
   unsigned int totalLen;
   int rc;
   int ret=0;

   buf->clientId    = g_clientId;
   buf->seq         = g_seq++;

   totalLen = sizeof(DevMsgHeader) + buf->dataLength;

   //rc = write(g_fd, buf, totalLen);
   rc = send(g_fd, buf, totalLen, 0);
   if (rc < 0)
   {
      if (errno == EPIPE)
      {
         printf("got EPIPE, dest app is dead\r\n");
         return -2;
      }
      else
      {
         printf("write failed, err=%d\r\n", strerror(errno));
         ret = -1;
      }
   }
   else if (rc != (int) totalLen)
   {
      printf("unexpected rc %d, expected %u", rc, totalLen);
      ret = -1;
   }

   return ret;
}
