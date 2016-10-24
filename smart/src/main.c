#include <sched.h>
#include "share.h"

#define MANUFACTURE FX
#define MODULE_NAME LED
#define SERVICE_NAME "feixunFlight"

enum
{
    TYPE,
    TRANSPORTTYPE,
    DEVICE_SN,
    CONFIG,
    __SEND_MAX
};

static struct ubus_context *smart_ctx;
static int handle_smart_feature_down(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

static int handle_send_update(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg);

static const struct blobmsg_policy handle_send_policy[] = {
    [TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
    [TRANSPORTTYPE] = { .name = "transportType", .type = BLOBMSG_TYPE_STRING },
    [DEVICE_SN] = { .name = "device_sn", .type = BLOBMSG_TYPE_STRING },
    [CONFIG] = { .name = "config", .type = BLOBMSG_TYPE_STRING },
};

static const struct ubus_method smart_object_methods[] = { 
    {.name = "disable", .handler = handle_smart_feature_down},
    UBUS_METHOD("send", handle_send_update, handle_send_policy),
};

static struct ubus_object_type smart_object_type =
UBUS_OBJECT_TYPE("smart", smart_object_methods);
/* policy means that you need to give a json value */

static struct ubus_object smart_object = { 
    .name = SERVICE_NAME,
    .type = &smart_object_type,
    .methods = smart_object_methods,
    .n_methods = ARRAY_SIZE(smart_object_methods),
};


int ubusInit(void);

int ubusInit(void)
{
    int ret = -1; 
    const char *ubus_socket = NULL;

    smart_debug("%s[%d]: Enter ", __func__, __LINE__);
    uloop_init();
    smart_ctx = ubus_connect(ubus_socket);

    if (!smart_ctx)
    {   
        smart_error("Failed to connect to ubus");
        return -1; 
    }   

    else
    {   
        smart_debug("%s[%d]: connect to ubus succeed, smart_ctx = %p", __func__, __LINE__, smart_ctx);
    }   

    ubus_add_uloop(smart_ctx); //Add the connected fd into epoll fd set

    ret = ubus_add_object(smart_ctx, &smart_object);

    if (ret)
    {

        smart_error("Failed to add object: %s", ubus_strerror(ret));
    }
    else
    {
        smart_debug("%s[%d]: ubus add object successfully, ret = %d", __func__, __LINE__, ret);
    }

    smart_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;

}

static int handle_smart_feature_down(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
{
    smart_debug("%s[%d]: Enter ", __func__, __LINE__);

    smart_debug("%s[%d]: need to add handle update func here ", __func__, __LINE__);

    smart_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

static int handle_send_update(struct ubus_context *ctx, struct ubus_object *obj,
        struct ubus_request_data *req, const char *method,
        struct blob_attr *msg)
{
    smart_debug("%s[%d]: Enter ", __func__, __LINE__);
    smart_debug("%s[%d]: msg = %s, len = %d ", __func__, __LINE__, blob_data(msg), blob_len(msg));

    struct blob_attr *tb[__SEND_MAX];
    char *type_ = NULL;
    char *transport_ = NULL;
    char *deviceSn_ = NULL;
    char *config_ = NULL;
    short nwkAddr;
    unsigned char *data = NULL;
    int len = 0;
    unsigned char sendBuf[SEND_MAX_BUF_LEN] = {0};
    int ret = 0;

    blobmsg_parse(handle_send_policy, ARRAY_SIZE(handle_send_policy), tb, blob_data(msg), blob_len(msg));

    if((tb[TYPE]) && (tb[TRANSPORTTYPE]) && (tb[DEVICE_SN]) && (tb[CONFIG]))
    {    
        type_ = blobmsg_data(tb[TYPE]);
        smart_debug("%s[%d]: type_ = %s", __func__, __LINE__, type_);

        transport_ = blobmsg_data(tb[TRANSPORTTYPE]);
        smart_debug("%s[%d]: transport_ = %s", __func__, __LINE__, transport_);

        deviceSn_ = blobmsg_data(tb[DEVICE_SN]);
        smart_debug("%s[%d]: deviceSn_ = %s", __func__, __LINE__, deviceSn_);

        config_ = blobmsg_data(tb[CONFIG]);
        smart_debug("%s[%d]: config_ = %s", __func__, __LINE__, config_);
    } else {
        smart_debug("%s[%d]: parameter wrong, please double check", __func__, __LINE__);
    }
    
    if (strcmp(transport_, "zigbee") == 0)
    {
        //sendBuf[0] = sizeof(deviceSn_);
        //memcpy(sendBuf + 1, deviceSn_, sizeof(deviceSn_));

        nwkAddr = getDevNWKAddrFromList(deviceSn_); 
        if (nwkAddr == -1)
        {
            smart_error("device isn't exist\r\n");
            printDevList();
            return -1;
        }
    
        len = sizeof(nwkAddr) + strlen(config_) + 1 + 2;
        data = (unsigned char *)malloc(len);

        data[0] = sizeof(nwkAddr);
        memcpy(data + 1, &nwkAddr, sizeof(nwkAddr));
        data[sizeof(nwkAddr) + 1] = strlen(config_) + 1;
        memcpy(data + sizeof(nwkAddr) + 2, config_, strlen(config_) + 1);

        assembleMTMsg(sendBuf, len, 0x29, 0x02, data);

        ret = write(zigbeeUloopFd.fd, sendBuf, len + 5);
        if (ret < 0) 
        {
            smart_error(" write zigbee ACM tty error!");
        }

        free(data);
    } 
 
    smart_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}



int main()
{
#ifdef HIGHEST_PRIOTITY
    struct sched_param smart_param;
    int policy;
    int max_priority, min_priority;

    policy = sched_getscheduler(0);
    smart_debug("use schedule plicy[%d]", policy);
    sched_getparam(0, &smart_param);
    smart_debug("schedule priority[%d]", smart_param.sched_priority);
    sched_get_priority_max(policy);
    sched_get_priority_min(policy);


    smart_param.sched_priority = 5;
    if (sched_setscheduler(0, SCHED_RR, &smart_param) == -1)
    {
        smart_debug("set schedule failed : %d", errno);
        return;
    }
#endif

    ubusInit();
    devIPUloopFdCreate();
    devZigbeeUloopFdCreate();
    devBluetoothUloopFdCreate();
    createSocketForConfigMessage();
    uloop_fd_add(&ipUloopFd, ULOOP_READ | ULOOP_BLOCKING);
    uloop_fd_add(&zigbeeUloopFd, ULOOP_READ | ULOOP_BLOCKING);
    uloop_fd_add(&bluetoothUloopFd, ULOOP_READ | ULOOP_BLOCKING);
    uloop_fd_add(&configFd, ULOOP_READ | ULOOP_BLOCKING);
    uloop_run();
    ubus_free(smart_ctx); //When request done, just free the resource, and return 
    uloop_done();

    return 0;
}
