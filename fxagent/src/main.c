#include <sched.h>
#include "share.h"
#include "parser.h" 
#include "cJSON.h" 
#include "proto.h"
#include "handle_sys_feature.h"
#include "socket.h"
#include "ubus.h"
#include "pthread.h"
#include "DataConvert.h"

//#define SUPPORT_HIGH_PRIORITY 1
#define NEED_REGISTER 1
#define NO_NEED_REGISTER 0
#define CLOUDC_GET_CONFIG "/etc/config/cloudc"
#define CLOUD_SERVER_URL_LEN 128

/** public data **/
#define MAX_DEV_ID_LEN 63
char g_devId[MAX_DEV_ID_LEN+1]={0};
char g_manufacture[32]={0};
char g_manufactureSN[64]={0};
unsigned int g_cmdId=1;

char ap_mac[MAX_MAC_LEN] = {0};
char ap_sn[MAX_SN_LEN] = {0};
char cloudc_server_ip[MAX_IP_LEN] = "115.29.49.52";
int cloudc_server_port = 80;
int ap_register_flag = 0;
/* attention: 
 * ap_register_flag need to set as 1 on officail release
 * now set it as 0 because it is easy to debug other func
 * */
extern void StartAlljoynService();
extern void StopAlljoynService();

/** function declare **/
int ap_check_register_condition(void);
//extern "C" int ap_register(void);
int config_init(int argc, char *argv[]);

int ap_check_register_condition(void)
{
    cloudc_debug("Enter.");
    /* This func is used to check whether the ap can register to server, such as below:
     * if wan is up?
     * ...
     * currently, as the interface required, no matter the ap is registered or not before,
     * it will send register request again when ap reboot
     *
     */
    cloudc_debug("Exit ");

    return NEED_REGISTER;
}

int ap_register(void)
{
    /* This func is used to register to cloud server.
     * After registered, ap can get some info which can be used to communicate with cloud server.
     * The info cloud server return may like the cloud_manage_ip, report_ip, report_port....
     * ...
     * if the ap is not registered, then will go to this func.
     * if ap cannot register succeed in this func, just register again and again .....
     *
     */

    cloudc_debug("Enter.");
    /* need to add how to register ==
     * need to register again and again until it register succeed
     */
    cloudc_send_register_buf();
    //uloop_timeout_set(&register_recv_timer, RECV_TIME_OUT * 1000);

    cloudc_debug("Exit.");

    return -1;
}

int config_init(int argc, char *argv[])
{
    int i = 0;
    char tmpStr[32] = {0};
    char *tmpServerIp = NULL;
    char *tmpServerPort = NULL;
    char serverStr[128]={0};

    cloudc_debug("Enter.");

    for(i = 0; i < argc; i ++)
    {
        cloudc_debug("%s[%d]: argv[%d] = %s\n", __func__, __LINE__, i, argv[i]);
    }

    if (argc == 1)
    {
        cloudc_error("need input server ip and port");
        return -1;

    }

    strncpy(tmpStr, argv[1], sizeof(tmpStr) - 1 );
    tmpServerIp = strtok(tmpStr,":");
    tmpServerPort = strtok(NULL, ":");

    if((NULL != tmpServerIp) && (NULL != tmpServerPort))
    {
        memset(cloudc_server_ip, 0, MAX_IP_LEN);
        strncpy(cloudc_server_ip, tmpServerIp, MAX_IP_LEN - 1);
        cloudc_server_port = atoi(tmpServerPort);
    }

    snprintf(serverStr, 128, "%s:%d", cloudc_server_ip, cloudc_server_port);
    serverStr[127]='\0';
    cloudc_debug("server=%s", serverStr);
    iota_genHostStr(serverStr);

    get_ap_info(); //get ap_mac and ap_sn

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

int main(int argc, char *argv[])
{
#ifdef SUPPORT_HIGH_PRIORITY
    struct sched_param fxagent_param;
    int policy;
    int max_priority, min_priority;

    policy = sched_getscheduler(0);
    cloudc_debug("fxagent use schedule plicy[%d]", policy);
    sched_getparam(0, &fxagent_param);
    cloudc_debug("fxagent schedule priority[%d]", fxagent_param.sched_priority);
    sched_get_priority_max(policy);
    sched_get_priority_min(policy);


    fxagent_param.sched_priority = 5;
    if (sched_setscheduler(0, SCHED_RR, &fxagent_param) == -1)
    {
        cloudc_debug("set fxagent schedule failed : %d", errno);
        return -1;
    }
#endif
    db_connect();
    config_init(argc, argv);
    cloudc_ubus_init();

    socket_init();
    //uloop_fd_add(&cloudc_monitor_uloop_fd, ULOOP_READ | ULOOP_BLOCKING); 
    uloop_fd_add(&cloudc_monitor_uloop_fd, ULOOP_READ | ULOOP_BLOCKING); 

    cloudc_pthread_init();

    if (ap_check_register_condition())
    {
        ap_register();
    }
    else
    {
        cloudc_debug("%s[%d]: ap already registered, so there is no need to register again", __func__, __LINE__);
    }

    StartAlljoynService();
    uloop_run();
    ubus_free(cloudc_ctx);
    uloop_done();
    StopAlljoynService();

    return 0;
}
