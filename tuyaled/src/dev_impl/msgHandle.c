#include "datatypes.h"
#include "msg.h"

void handle_devOnline(client_t *c, DevMsgHeader *msg)
{
}

void devMsg_handle(client_t *c, DevMsgHeader *msg)
{
    switch(msg->type) 
    {
        case DEV_MSG_DEV_ONLINE:
            handle_devOnline(c, msg);
            break;

        default:
            break;
    }
}
