#include "iotc.h"

/** global data **/
unsigned int g_cmdId=1;

char iotc_server_ip[MAX_IP_LEN]="115.29.49.52";
int iotc_server_port=80;

extern int connection_init(void);

static int config_init(int argc, char *argv[])
{
    int i=0;
    char tmpStr[32]={0};
    char *tmpServerIp=NULL;
    char *tmpServerPort=NULL;
    char serverStr[128]={0};

    iotc_debug("Enter.");

    for(i = 0; i < argc; i ++)
    {
        if(1 == i)
        {
            strncpy(tmpStr, argv[i], sizeof(tmpStr) - 1 );
            tmpServerIp = strtok(tmpStr,":");
            tmpServerPort = strtok(NULL, ":");

            if((NULL != tmpServerIp) && (NULL != tmpServerPort))
            {
                memset(iotc_server_ip, 0, MAX_IP_LEN);
                strncpy(iotc_server_ip, tmpServerIp, MAX_IP_LEN - 1);
                iotc_server_port = atoi(tmpServerPort);
            }

            break;
        }
    }

    snprintf(serverStr, 128, "%s:%d", iotc_server_ip, iotc_server_port);
    serverStr[127]='\0';
    iotc_debug("server=%s", serverStr);
    iotc_genHostStr(serverStr);

    iotc_debug("Exit.");

    return 0;
}

struct uloop_timeout g_sendDevOnlineTm = {
    .cb = iotc_sendDevOnlineMsg,
};

int main(int argc, char *argv[])
{
    config_init(argc, argv);

    if (glue_init() != 0)
    {
        iotc_error("glue init failed!");
        return -1;
    }

    if (0 != connection_init())
    {
        iotc_error("socket_init failed!");
        return -1;
    }

    uloop_init();
    uloop_fd_add(&iotc_monitor_uloop_fd, ULOOP_READ);
    uloop_timeout_set(&g_sendDevOnlineTm, 2000);
    uloop_run();
    uloop_done();

    return 0;
}
