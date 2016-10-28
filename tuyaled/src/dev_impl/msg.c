#include "datatypes.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAX_DEV_MSG_LEN 512

int devMsg_initServerSocket(unsigned int port)
{
    struct sockaddr_in server;
    int fd=-1, rc;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket");
        return fd;
    }

    /*
     * Bind my server address and listen.
     */
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    rc = bind(fd, (struct sockaddr *) &server, sizeof(server));
    if (rc != 0)
    {
        printf("bind to port %d failed, rc=%d errno=%d\r\n", port, rc, errno);
        close(fd);
        return -1;
    }

    rc = listen(fd, 3);
    if (rc != 0)
    {
        printf("listen to port %s failed, rc=%d errno=%d\r\n", port, rc, errno);
        close(fd);
        return -1;
    }

    printf("msg socket opened and ready (fd=%d)\r\n", fd);

    return fd;
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

    rc = read(fd, msg, sizeof(DevMsgHeader));
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
        rc = read(fd, inBuf, msg->dataLength);
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

int devMsg_send(client_t *c, DevMsgHeader *buf)
{
   unsigned int totalLen;
   int rc;
   int ret=0;

   buf->clientId    = c->clientId;
   buf->seq         = c->seq++;

   totalLen = sizeof(DevMsgHeader) + buf->dataLength;

   rc = write(c->fd_, buf, totalLen);
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
      printf("unexpected rc %d, expected %u", rc, totalLen);
      ret = -1;
   }

   return ret;
}
