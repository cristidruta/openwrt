#include "datatypes.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "msg.h"
#include "slist.h"
#include <string.h>
#include "../log.h"
extern void devMsg_handle(client_t *c, DevMsgHeader *msg);

static void clients_delete_element(void* e)
{
    if(!e)
        return;

    client_t* element = (client_t*)e;
    if(element->fd_ >= 0)
        close(element->fd_);

    free(e);
}

int clients_init(clients_t* list)
{
    return slist_init(list, &clients_delete_element);
}

void clients_clear(clients_t* list)
{
    slist_clear(list);
}

static int clients_add(clients_t* list, int fd)
{
    if(!list)
        return -1;

    client_t* element = (client_t*) malloc(sizeof(client_t));
    if(!element) {
        close(fd);
        return -2;
    }

    memset(element, 0, sizeof(client_t));
    element->fd_ = fd;

    if(slist_add(list, element) == NULL) {
        close(element->fd_);
        free(element);
        return -2;
    }

    return 0;
}

void clients_read_fds(clients_t* list, fd_set* set, int* max_fd)
{
    if(!list)
        return;

    slist_element_t* tmp = list->first_;
    while(tmp) {
        client_t* l = (client_t*)tmp->data_;
        FD_SET(l->fd_, set);
        *max_fd = *max_fd > l->fd_ ? *max_fd : l->fd_;
        tmp = tmp->next_;
    }
}

//client_t *clients_getFirstNode(clients_t* list, char *ManufactureSN, char *Manufacture)
client_t *clients_getNode(clients_t* list, char *ManufactureSN, char *Manufacture)
{
    if(!list)
        return NULL;

    slist_element_t* tmp = list->first_;
    while(tmp) {
        client_t* l = (client_t*)tmp->data_;
        if (strcmp(l->manufactureSN, ManufactureSN) == 0 && strcmp(l->manufacture, Manufacture)== 0)
        {
            return l;
        }
        tmp = tmp->next_;
    }
    if (!tmp)
    {
        adapt_debug("Donnot find the node!");
        return NULL;

    }
    
}

int clients_handle_accept(int fd, clients_t* list, fd_set* set)
{
    struct sockaddr_in client;
    unsigned int sockAddrSize=sizeof(client);
    int client_fd=-1;

    if(!FD_ISSET(fd, set)) {
        return 0;
    }

    if ((client_fd = accept(fd, (struct sockaddr *)&client, &sockAddrSize)) < 0)
    {
        adapt_debug("accept IPC connection failed. errno=%d\r\n", errno);
        return -1;
    }

    return clients_add(list, client_fd);
}

int clients_read(clients_t* list, fd_set* set)
{
    if(!list)
        return -1;

    slist_element_t* tmp = list->first_;
    while(tmp) {
        DevMsgHeader *msg=NULL;
        client_t* c = (client_t*)tmp->data_;
        tmp = tmp->next_;
        if(!FD_ISSET(c->fd_, set)) {
            continue;
        }

        if (devMsg_receive(c->fd_, &msg) < 0) {
            adapt_debug("client %d read error, removing it\r\n", c->fd_);
            slist_remove(list, c);
            break;
        }

        devMsg_handle(c, msg);

        free(msg);
    }

    return 0;
}
