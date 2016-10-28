#include "datatypes.h"
#include "msg.h"
#include "clients.h"

#define DEV_SERVER_PORT 9999

int g_listenFd=-1;
clients_t clients;

void devMgmt_clean();
void devMgmt_loop()
{
    fd_set readfds;
    int nfds=g_listenFd;
    int ret=0;

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(g_listenFd, &readfds);
        nfds=g_listenFd;

        clients_read_fds(&clients, &readfds, &nfds);

        ret = select(nfds+1, &readfds, NULL, NULL, NULL);
        if (ret == -1) {
            printf("select returned with error: %s\r\n", strerror(errno));
            break;
        }

        if ((ret=clients_handle_accept(g_listenFd, &clients, &readfds)) != 0)
        {
            printf("clients_handle_accept error, break\r\n");
        }

        if ((ret=clients_read(&clients, &readfds)) != 0)
        {
            printf("clients_read error, break\r\n");
        }
    }

    devMgmt_clean();
}

int devMgmt_init()
{
    if (clients_init(&clients) < 0)
    {
        printf("clients_init failed!\r\n");
        return -1;
    }

    if ((g_listenFd=devMsg_initServerSocket(DEV_SERVER_PORT)) < 0)
    {
        printf("devMsg_initServerSocket failed!\r\n");
        return -1;
    }

    printf("devMgmt_init successful!\r\n");
    return 0;
}

void devMgmt_clean()
{
    if (g_listenFd > 0)
    {
        close(g_listenFd);
    }
    clients_clear(&clients);
}
