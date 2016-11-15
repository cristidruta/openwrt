
/* This file is used to buid packet as defined and send to server.
 * */
#include "share.h"
#include "parser.h"
#include "handle_sys_feature.h"
#include "cJSON.h"

#define POSTHEADER_WITHOUT_CONTENT "POST /gateway HTTP/1.1\r\n\
Host: %s:%d\r\n\
User-Agent: Mozilla/5.0\r\n\
Accept: text/html, application/json\r\n\
Accept-Language: en-US\r\n\
Accept-Encoding: gzip, deflate\r\n\
Connection: keep-alive\r\n\
Content-Type: application/json\r\n\
Content-Length: %d\r\n\
\r\n"

extern char g_devId[];
extern unsigned int g_cmdId;

/* iota func start */
#define HTTP_HEAD_FIX "POST /gateway HTTP/1.1\r\n\
User-Agent: iota/1.0\r\n\
Accept: text/html, application/json\r\n\
Accept-Language: en-US\r\n\
Accept-Encoding: gzip, deflate\r\n\
Connection: keep-alive\r\n\
Content-Type: application/json\r\n"

#define HTTP_HEAD_HOST "Host: %s\r\n"
#define HTTP_HEAD_CON_LEN "Content-Length: %5d\r\n\r\n"


#define MAX_HOST_STR_LEN 512
static char g_hostStr[MAX_HOST_STR_LEN];
static int g_hostHdrLen=-1;


#define HEAD_FIX_LEN (strlen(HTTP_HEAD_FIX))
#define HEAD_CON_LEN_LEN (25)
#define HEAD_HOST_LEN (g_hostHdrLen)

#define FXAGENT_HEAD_LEN (sizeof(struct fxIoT_head))

static int iota_addIoTHeader(char *buf, int conLen)
{
    struct fxIoT_head head;
    memset(&head, 0, sizeof(struct fxIoT_head));

    head.length = conLen;

    strncpy(buf, &head, FXAGENT_HEAD_LEN);
    *(buf + FXAGENT_HEAD_LEN + conLen) = '\0';

    return 0;
}

void iota_genHostStr(char *server)
{
    g_hostHdrLen = snprintf(g_hostStr, sizeof(g_hostStr)-1, HTTP_HEAD_HOST, server);
    g_hostStr[MAX_HOST_STR_LEN-1] = '\0';
}

static int iota_getHttpHeadLen()
{
    if(HEAD_HOST_LEN == -1) 
    {
        return -1;
    }

    return (HEAD_FIX_LEN+HEAD_HOST_LEN+HEAD_CON_LEN_LEN);
}

static int iota_addHttpHeader(char *buf, int conLen)
{
    strncpy(buf, HTTP_HEAD_FIX, HEAD_FIX_LEN);
    strncpy(buf+HEAD_FIX_LEN, g_hostStr, HEAD_HOST_LEN);
    snprintf(buf+HEAD_FIX_LEN+HEAD_HOST_LEN, HEAD_CON_LEN_LEN, HTTP_HEAD_CON_LEN, conLen);
    *(buf+HEAD_FIX_LEN+HEAD_HOST_LEN+HEAD_CON_LEN_LEN-1)='\n';

    return 0;
}

/* success: return http body length 
 * failure: return -1 */
static int iota_buildSetRspData(char *buf, int maxLen, int cmdId, int retCode, cJSON *jsonDevData)
{
    int ret=-1;
    cJSON *jsonRoot=NULL;
    char *s=NULL;
    int needDetachJsonDevData=0;

    cloudc_error("Enter.");

    /*create json string root*/
    if ((jsonRoot=cJSON_CreateObject()) == NULL)
    {
        cloudc_error("create json root node failed!");
        goto exit;
    }

    cJSON_AddStringToObject(jsonRoot, "type", "rsp_set");
    cJSON_AddNumberToObject(jsonRoot, "commandId", cmdId);
    cJSON_AddStringToObject(jsonRoot, "deviceId", g_devId);
    cJSON_AddNumberToObject(jsonRoot, "ret", retCode);
       
    if (0 == retCode)
    {
        if (jsonDevData)
        {
            cJSON_AddItemToObject(jsonRoot, "devData", jsonDevData);
            needDetachJsonDevData=1;
        }
    }

    if ((s=cJSON_PrintUnformatted(jsonRoot)) == NULL)
    {
        cloudc_error("convert json to string format failed!");
        goto exit1;
    }

    ret=snprintf(buf, maxLen, "%s", s);
    buf[maxLen-1] = '\0';
    cloudc_debug("buf=%s\n", buf);
    free(s);

    if (needDetachJsonDevData == 1)
    {
        cJSON_DetachItemFromObject(jsonRoot, "devData");
    }

exit1:
    cJSON_Delete(jsonRoot);
exit:
    cloudc_debug("Exit.");
    return ret;
}

static int iota_sendHttpBuf(char *buf, int len)
{
    int ret=-1;

    cloudc_debug("Enter.");

    if ( NULL==buf || len < 10 )
    {
        cloudc_error("param error! len=%d", len);
        return -1;
    }

    if (-1 == is_socket_connected(cloudc_monitor_uloop_fd.fd))
    {
        close(cloudc_monitor_uloop_fd.fd);
        //socket_init();
    }

    ret=send(cloudc_monitor_uloop_fd.fd, buf, len, 0);

    if (ret > 0)
    {
        cloudc_debug("send http buf succeed!");
        return 0;
    }
    else if (ret == -1)
    {
        cloudc_error("send msg failed! %s!", strerror(errno));
        close(cloudc_monitor_uloop_fd.fd);
        //socket_create();
        //socket_connect();
        /* TODO: how to handle send fail? */
        //iotc_send_http_buf(buf);
    }
    else
    {
        cloudc_error("send msg failed! ret=%d, %s.", ret, strerror(errno));
        return -1;
    }
    cloudc_debug("Exit.");
}
/* iota func end */

int cloudc_build_register_js_buf(char *buf, int maxLen); 
int cloudc_build_online_js_buf(char *js_buf, char *devData); 
int cloudc_build_recv_rsp_js_buf(char *type, int serial, int rsp_status, char *js_buf);
int cloudc_build_alljoyn_recv_rsp_js_buf(char *type, int serial, char *user_id, char *device_id, int rsp_status, char *js_buf);
int cloudc_build_notification_js_buf(char *type, int serial, char *deviceId,char *versionId, char *path, char *js_buf);//add by whzhe
int cloudc_build_rsp_ipk_js_buf(char *type, int serial, struct ipk_info *ipk_list_head, int *status, int real_ipk_num, char *js_buf);
int cloudc_build_rsp_query_js_buf(char *type, int serial, struct ipk_query_info_node *query_list_head, char *js_buf);
int cloudc_build_rsp_opkg_js_buf(char *type, int serial, int update_status, int replace_status, char *js_buf);
int cloudc_build_rsp_plugin_js_buf(char *buf, int maxLen, char *type, int serial, char *action, int ret, char *deviceId, int pluginId, char *plugin_version);
int cloudc_build_rsp_get_js_buf(char *type, int serial, char *user_id, char *device_id, char *device_type, struct ipk_info *config_info_head, int key_name_num, char *js_buf); 
int cloudc_build_rsp_set_js_buf(char *type, int serial, char *user_id, char *device_id, char *devData, int retCode, char *js_buf);
int cloudc_build_send_http_buf(char *js_buf, char *http_buf);

int cloudc_send_register_buf(void); 
int cloudc_send_online_buf(char *devData); 
int cloudc_send_recv_rsp_buf(char *type, int serial, int rsp_status);
int cloudc_send_alljoyn_recv_rsp_buf(char *type, int serial, char *user_id, char *device_id, int rsp_status);
int cloudc_send_http_buf(char *http_buf);
int cloudc_send_rsp_ipk_buf(char *type, int serial, struct ipk_info *ipk_list_head, int *status, int real_ipk_num);
int cloudc_send_rsp_query_buf(char *type, int serial, struct ipk_query_info_node *query_list_head);
int cloudc_send_rsp_get_buf(char *type, int serial, char *user_id, char *device_id, char *device_type, struct ipk_info *config_info_head, int key_name_num);
int cloudc_send_rsp_set_buf(char *type, int cmdId, char *user_id, char *device_id, char *devData, int retCode);
int cloudc_send_rsp_device_online_buf(char *type, int serial, char *user_id, char *device_id, char *devData, struct ipk_info *config_info_head, int key_name_num);
int cloudc_send_rsp_opkg_buf(char *type, int serial, int update_status, int replace_status); 

int cloudc_build_send_http_buf(char *js_buf, char *http_buf)
{
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);

    snprintf(http_buf, SEND_MAX_BUF_LEN, POSTHEADER_WITHOUT_CONTENT, cloudc_server_ip, cloudc_server_port, strlen(js_buf));
    strncat(http_buf, js_buf, (SEND_MAX_BUF_LEN - strlen(http_buf)));

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

int cloudc_send_http_buf(char *http_buf)
{
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);
    int ret = -1;

    if (-1 == is_socket_connected(cloudc_monitor_uloop_fd.fd))
    {
        close(cloudc_monitor_uloop_fd.fd);
        socket_init();
        ap_register();
    }

    if (NULL != http_buf)
    {
        //pthread_mutex_lock(&mutex);
        //ret = send(cloudc_monitor_uloop_fd.fd, http_buf, SEND_MAX_BUF_LEN, 0);
        ret = send(cloudc_monitor_uloop_fd.fd, (char *)http_buf, strlen(http_buf), MSG_DONTWAIT);
        //uloop_timeout_set(&send_timer, SEND_TIME_OUT * 1000);

        //pthread_mutex_unlock(&mutex);

        if (ret > 0)
        {
            cloudc_debug("%s[%d]: ret = %d, client send http buf succeed", __func__, __LINE__, ret);
            cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
            return 0;
        }
        /*else if (ret = -1)
        {
            cloudc_error("%s[%d]: send() failed: ret = %d, %s!", __func__, __LINE__, ret, strerror(errno));
            close(cloudc_monitor_uloop_fd.fd);
            socket_create();
            socket_connect();
            ap_register();
            cloudc_send_http_buf(http_buf);
        }*/
        else
        {
            cloudc_error("%s[%d]: send() failed: ret = %d, %s!", __func__, __LINE__, ret, strerror(errno));
            cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
            return -1;
        }
    }

    else
    {
        cloudc_error("%s[%d]: http_buf is null, please double confirm!", __func__, __LINE__);
        cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
        return -1;
    }
}

int cloudc_send_rsp_ipk_buf(char *type, int serial, struct ipk_info *ipk_list_head, int *status, int real_ipk_num)
{
    cloudc_error("%s[%d]: Enter!", __func__, __LINE__);
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    ret1 = cloudc_build_rsp_ipk_js_buf(type, serial, ipk_list_head, status, real_ipk_num, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);

    }

    cloudc_error("%s[%d]: Exit!", __func__, __LINE__);
    return ret1;
}

int cloudc_send_rsp_query_buf(char *type, int serial, struct ipk_query_info_node *query_list_head)
{
    cloudc_error("%s[%d]: Enter!", __func__, __LINE__);
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    ret1 = cloudc_build_rsp_query_js_buf(type, serial, query_list_head, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return ret1;
}

int cloudc_send_rsp_get_buf(char *type, int serial, char *user_id, char *device_id, char *device_type, struct ipk_info *config_info_head, int key_name_num)
{
    cloudc_error("%s[%d]: Enter!", __func__, __LINE__);
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    ret1 = cloudc_build_rsp_get_js_buf(type, serial, user_id, device_id, device_type, config_info_head, key_name_num, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return ret1;
}

int cloudc_send_rsp_set_buf(char *type, int cmdId, char *user_id, char *device_id, char *devData, int retCode)
{
#if 0
    //char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    //char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;
    char *build_http_buf1="{\"type\":\"rsp_set\",\"commandId\":14,\"userId\":\"3\",\"deviceId\":\"15\",\"status\":2,\"ret\":0,\"devData\":{\"power\":\"1\"}}";

    cloudc_debug("%s[%d]: Enter!", __func__, __LINE__);

    char *build_js_buf = malloc(SEND_MAX_BUF_LEN);
    char *build_http_buf = malloc(SEND_MAX_BUF_LEN);

    //memset(build_js_buf, 0, SEND_MAX_BUF_LEN);
    //memset(build_http_buf, 0, SEND_MAX_BUF_LEN);

    //ret1 = cloudc_build_rsp_set_js_buf(type, serial, user_id, device_id, devData, retCode, build_js_buf);
    //build_js_buf[SEND_MAX_BUF_LEN-1] = '\0';
    strcpy(build_js_buf, build_http_buf1);
    //if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        build_http_buf[SEND_MAX_BUF_LEN-1] = '\0';
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }
    free(build_js_buf);
    free(build_http_buf);
    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return ret1;
#endif


    int ret=-1;
    int headLen=0, conLen=-1;
    char httpBuf[SEND_MAX_BUF_LEN]={0};
    cJSON * jsonDevData=NULL;

    cloudc_debug("Enter.");

    if ((jsonDevData=cJSON_Parse(devData)) == NULL)
    {
        cloudc_error("failed to pass devData %s!", devData);
        return -1;
    }

#if 0
    if ((headLen=iota_getHttpHeadLen()) == -1)
    {
        cloudc_error("iot server string is missing!");
        return -1;
    }    
#endif
    /* reserve headLen bytes for http head */
    if ((conLen=iota_buildSetRspData(&httpBuf[FXAGENT_HEAD_LEN],
                                     SEND_MAX_BUF_LEN-FXAGENT_HEAD_LEN,
                                     cmdId,
                                     retCode,
                                     jsonDevData)) < 0)
    {
        cloudc_error("failed to build http body!");
        return -1;
    }

#if 0
    if ((ret=iota_addHttpHeader(httpBuf, conLen)) != 0)
    {
        cloudc_error("failed to build http head!");
        return -1;
    }
#else
    iota_addIoTHeader(httpBuf, conLen);
#endif
    cloudc_debug("httpBuf=%s", httpBuf);

    iota_sendHttpBuf(httpBuf, FXAGENT_HEAD_LEN+conLen);

    cloudc_debug("Exit.");

    return ret;    
}

int cloudc_build_rsp_query_js_buf(char *type, int serial, struct ipk_query_info_node *query_list_head, char *js_buf)
{
    struct ipk_query_info_node *query_list_tmp = query_list_head->pNext;
    cJSON *root;
    /*create json string root*/
    root = cJSON_CreateObject();

    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);

    if (!root) 
    {   
        cloudc_debug("%s[%d]: get root faild !", __func__, __LINE__);
        cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
        goto EXIT;
    }   
    else 
    {   
        cloudc_debug("%s[%d]: get root success! ", __func__, __LINE__);
    }   

    {   
        cJSON * js_body ;
        char rsp_type[40] = {0};
        snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

        const char *const body = "list"; 
        cJSON_AddStringToObject(root, "type", rsp_type); 
        cJSON_AddNumberToObject(root, "commandId", serial); 
        cJSON_AddItemToObject(root, body, js_body= cJSON_CreateArray()); 

        while (NULL != query_list_tmp)
        {
            cJSON_AddStringToObject(js_body, "name", query_list_tmp->ipk_query_name);
            cloudc_debug("%s[%d]: install_ipk_list = %s", __func__, __LINE__, query_list_tmp->ipk_query_name);
            query_list_tmp = query_list_tmp->pNext;
        }

        {
            char *s = cJSON_PrintUnformatted(root);
            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(root);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
EXIT:
    return -1;
}

int cloudc_build_rsp_ipk_js_buf(char *type, int serial, struct ipk_info *ipk_list_head, int *status, int real_ipk_num, char *js_buf)
{
    cloudc_error("%s[%d]: Enter!", __func__, __LINE__);
    cJSON *json_root;
    struct ipk_info *ipk_list_tmp = ipk_list_head;

    /*create json string root*/
    json_root = cJSON_CreateObject();

    if (!json_root)
    {
        cloudc_error("%s[%d]: get json_root faild !\n", __func__, __LINE__);
        goto EXIT;
    }

    else
    {
        cloudc_debug("%s[%d]: get json_root succeed !\n", __func__, __LINE__);
    }

    {
        cJSON * js_array, *js_body ;

        char rsp_type[30] = {0};
        snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

        cJSON_AddStringToObject(json_root, "type", rsp_type);
        cJSON_AddNumberToObject(json_root, "commandId", serial);
        cJSON_AddItemToObject(json_root, "status", js_array= cJSON_CreateArray());

        int i = 0;
        for (i = 0; i < real_ipk_num; i ++)
        {            
            cJSON_AddItemToArray(js_array, js_body = cJSON_CreateObject());
            cJSON_AddItemToObject(js_body, "name", cJSON_CreateString(ipk_list_tmp->op_ipk_name));
            cJSON_AddItemToObject(js_body, "st", cJSON_CreateNumber(status[i]));
            cloudc_debug("%s[%d]: ############# ipk_name[%d] = %s, status[%d] = %d", __func__, __LINE__, i, ipk_list_tmp->op_ipk_name, i, status[i]);
            ipk_list_tmp = ipk_list_tmp->next;
        }

        {
            char *s = cJSON_PrintUnformatted(json_root);

            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }

    return 0;
EXIT:
    return -1;
}


int cloudc_build_register_js_buf(char *buf, int maxLen)
{
    cJSON *json_root=NULL;
    cJSON *jsonDevData=NULL;
    int ret = -1;
    char *s=NULL;
    char *devData = "{\"name\":\"Gateway\", \
                       \"manufacture\":\"feixun\", \
                       \"manufactureSN\":\"FXGW00000001\", \
                       \"manufactureDataModelId\":\"12\", \
                       \"deviceType\":\"Gateway\", \
                       \"softwareVersion\":\"V1.0.1\"}";

    cloudc_debug("Enter.");

    /*create json string root*/
    json_root = cJSON_CreateObject();

    if (!json_root) 
    {   
        cloudc_debug("get json_root faild !");
        return -1;
    }   

    cJSON_AddStringToObject(json_root, "type", "device_online");
    cJSON_AddNumberToObject(json_root, "commandId", g_cmdId++);

    /* TODO: need get devData from somewhere */
    jsonDevData = cJSON_Parse(devData);
    if(NULL == jsonDevData)
    {
        cloudc_error("devData is wrong!");
        cJSON_Delete(json_root);
        return -1;
    }

    cJSON_AddStringToObject(json_root, "deviceId", g_devId);
    cJSON_AddItemToObject(json_root, "devData", jsonDevData);

    s = cJSON_PrintUnformatted(json_root);
    if (s)
    {
        ret=snprintf(buf, maxLen, "%s", s);
        buf[maxLen-1] = '\0';
        cloudc_debug("ret=%d,buf=%s\n", ret, buf);
        free(s);
    }
    else
    {
        cloudc_error("convert json to string format failed!");
    }
    cJSON_Delete(json_root);

    cloudc_debug("Exit.");
    return ret;
}

int cloudc_build_online_js_buf(char *js_buf, char *devData)
{
    cJSON *json_root=NULL;
    cJSON *js_array=NULL, *js_body=NULL;
    cJSON *jsonDevData=NULL;
    char *s=NULL;

    cloudc_debug("Enter.");

    /*create json string root*/
    json_root = cJSON_CreateObject();
    if (!json_root) 
    {   
        cloudc_debug("%s[%d]: get json_root faild !", __func__, __LINE__);
        return -1;
    }   

    cJSON_AddStringToObject(json_root, "type", "device_online"); 
    cJSON_AddNumberToObject(json_root, "commandId", g_cmdId++);

    cloudc_debug("devData=%s", devData);
    jsonDevData = cJSON_Parse(devData);
    if(NULL == jsonDevData)
    {
        cloudc_error("devData is wrong!");
        cJSON_Delete(json_root);
        return -1; 
    }
    cJSON_AddItemToObject(json_root, "devData", jsonDevData);

    s = cJSON_PrintUnformatted(json_root);
    if (s)
    {
        strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
        cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
        free(s);
    }
    else
    {
        cloudc_error("convert json to string format failed!");
    }

    cJSON_Delete(json_root);
    cloudc_debug("Exit.");
    return 0;
}

int cloudc_send_register_buf(void)
{   
#if 0
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    cloudc_debug("Enter.");

    ret1 = cloudc_build_register_js_buf(build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_http_buf = %s", __func__, __LINE__, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("Exit.");
    return ret1;
#endif

    int conLen = -1;
    char tcpBuf[SEND_MAX_BUF_LEN]={0};
    cJSON * jsonDevData=NULL;

    if ((conLen = cloudc_build_register_js_buf(&tcpBuf[FXAGENT_HEAD_LEN],
                                          SEND_MAX_BUF_LEN - FXAGENT_HEAD_LEN)) < 0)
    {
        cloudc_error("failed to build tcp body!");
        return -1;
    }

    iota_addIoTHeader(tcpBuf, conLen);

    iota_sendHttpBuf(tcpBuf, FXAGENT_HEAD_LEN+conLen);

    cloudc_debug("Exit.");

    return 0;  
}

int cloudc_send_online_buf(char *devData)
{
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);

    ret1 = cloudc_build_online_js_buf(build_js_buf, devData);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_http_buf = %s", __func__, __LINE__, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
}

int cloudc_build_notification_js_buf(char *type, int serial, char *deviceId,char *userId, char *path, char *js_buf)
{
    cJSON *json_root;
    //create json string root
    json_root = cJSON_CreateObject();
    
    char noti_type[50] = {0};
    snprintf(noti_type, sizeof(noti_type), "noti_%s", type);

    if (!json_root)
    {
        cloudc_debug("%s[%d]: get json_root faild !", __func__, __LINE__);
        goto EXIT;
    }
    else
    {
        cloudc_debug("%s[%d]: get json_root success!", __func__, __LINE__);
    }
    {
        cJSON_AddStringToObject(json_root, "type", noti_type);
        cJSON_AddNumberToObject(json_root, "commandId", serial);
        cJSON_AddStringToObject(json_root, "deviceId", deviceId);
        cJSON_AddStringToObject(json_root, "userId", userId);
        cJSON_AddStringToObject(json_root, "IpmPath", path);
        {
            char *s = cJSON_PrintUnformatted(json_root);
            if (s)
            {
                strncpy(js_buf, s, NOTI_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }
    return 0;
EXIT:
    return -1;
}
int cloudc_build_recv_rsp_js_buf(char *type, int serial, int rsp_status, char *js_buf)
{
    cJSON *json_root;
    /*create json string root*/
    json_root = cJSON_CreateObject();

    char rsp_type[30] = {0};
    snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

    if (!json_root) 
    {   
        cloudc_debug("%s[%d]: get json_root faild !", __func__, __LINE__);
        goto EXIT;
    }   
    else 
    {   
        cloudc_debug("%s[%d]: get json_root success, rsp_status = %d!", __func__, __LINE__, rsp_status);
    }   

    {   
        cJSON_AddStringToObject(json_root, "type", rsp_type); 
        cJSON_AddNumberToObject(json_root, "commandId", serial); 
        cJSON_AddNumberToObject(json_root, "status", rsp_status); 
        {   
            char *s = cJSON_PrintUnformatted(json_root);
            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }
    return 0;
EXIT:
    return -1;
}

int cloudc_build_alljoyn_recv_rsp_js_buf(char *type, int serial, char *user_id, char *device_id, int rsp_status, char *js_buf)
{
    cJSON *json_root;
    /*create json string root*/
    json_root = cJSON_CreateObject();

    char rsp_type[30] = {0};
    snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

    if (!json_root) 
    {   
        cloudc_debug("%s[%d]: get json_root faild !", __func__, __LINE__);
        goto EXIT;
    }   
    else 
    {   
        cloudc_debug("%s[%d]: get json_root success, rsp_status = %d!", __func__, __LINE__, rsp_status);
    }   

    {   
        cJSON_AddStringToObject(json_root, "type", rsp_type); 
        cJSON_AddNumberToObject(json_root, "commandId", serial); 
        cJSON_AddStringToObject(json_root, "userId", user_id); 
        cJSON_AddStringToObject(json_root, "deviceId", device_id); 
        cJSON_AddNumberToObject(json_root, "status", rsp_status); 
        {   
            char *s = cJSON_PrintUnformatted(json_root);
            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }
    return 0;
EXIT:
    return -1;
}

int cloudc_send_recv_rsp_buf(char *type, int serial, int rsp_status)
{   
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);


    ret1 = cloudc_build_recv_rsp_js_buf(type, serial, rsp_status, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return ret1;
}

int cloudc_send_alljoyn_recv_rsp_buf(char *type, int serial, char *user_id, char *device_id, int rsp_status)
{
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;

    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);


    ret1 = cloudc_build_alljoyn_recv_rsp_js_buf(type, serial, user_id, device_id, rsp_status, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return ret1;
}

int cloudc_send_rsp_plugin_buf(char *type, int serial, char *action, int action_status, char *deviceId, int pluginId, char *plugin_version)
{
#if 0
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;
    
    cloudc_debug("Enter ");

    ret1 = cloudc_build_rsp_plugin_js_buf(type, serial, action, action_status, deviceId, pluginId, plugin_version, build_js_buf);

    if (0 == ret1)
    {
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("build_js_buf = %s, \nbuild_http_buf = %s",build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }
    return ret1;
#endif

    int conLen = -1;
    char tcpBuf[SEND_MAX_BUF_LEN]={0};
    cJSON * jsonDevData=NULL;

    if ((conLen = cloudc_build_rsp_plugin_js_buf(&tcpBuf[FXAGENT_HEAD_LEN],
                    SEND_MAX_BUF_LEN - FXAGENT_HEAD_LEN,
                    type, serial, action, action_status, 
                    deviceId, pluginId, plugin_version)) < 0)
    {
        cloudc_error("failed to build tcp body!");
        return -1;
    }

    iota_addIoTHeader(tcpBuf, conLen);

    iota_sendHttpBuf(tcpBuf, FXAGENT_HEAD_LEN+conLen);

    cloudc_debug("Exit.");

    return 0;  

}

int cloudc_build_rsp_plugin_js_buf(char *buf, int maxLen, char *type, int serial, char *action, int action_status, char *deviceId, int pluginId, char *plugin_version)
{
    cJSON *json_root;
    int ret = -1;
    /*create json string root*/
    json_root = cJSON_CreateObject();

    char rsp_type[30] = {0};
    snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

    if (!json_root) 
    {   
        cloudc_debug("get json_root faild !");
        goto EXIT;
    }   
    else 
    {   
        cloudc_debug("get json_root success!");
    }   

    cJSON_AddStringToObject(json_root, "type", rsp_type); 
    cJSON_AddNumberToObject(json_root, "commandId", serial); 
    cJSON_AddStringToObject(json_root, "action", action); 
    cJSON_AddStringToObject(json_root, "deviceId", deviceId); 
    cJSON_AddNumberToObject(json_root, "pluginId", pluginId); 
    cJSON_AddNumberToObject(json_root, "ret", action_status); 

    if (0 != strcasecmp("delete", action))
    {
        cJSON_AddStringToObject(json_root, "version", plugin_version); 
    }

    char *s = cJSON_PrintUnformatted(json_root);
    if (s)
    {
        ret=snprintf(buf, maxLen, "%s", s);
        buf[maxLen-1] = '\0';
        cloudc_debug("ret=%d,buf=%s\n", ret, buf);
        free(s);
    }
    cJSON_Delete(json_root);

    return ret;
EXIT:
    return -1;

}

int cloudc_send_rsp_opkg_buf(char *type, int serial, int update_status, int replace_status)
{
    char build_js_buf[SEND_MAX_BUF_LEN] = {0};
    char build_http_buf[SEND_MAX_BUF_LEN] = {0};
    int ret1 = -1;
    
    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);


    ret1 = cloudc_build_rsp_opkg_js_buf(type, serial, update_status, replace_status, build_js_buf);

    if (0 == ret1)
    {
        /* gaojing: need to sprintf http package then send */
        cloudc_build_send_http_buf(build_js_buf, build_http_buf);
        cloudc_debug("%s[%d]: build_js_buf = %s, \nbuild_http_buf = %s", __func__, __LINE__, build_js_buf, build_http_buf);

        cloudc_send_http_buf(build_http_buf);
    }
    return ret1;
}

int cloudc_build_rsp_opkg_js_buf(char *type, int serial, int update_status, int replace_status, char *js_buf)
{
    cJSON *json_root;
    /*create json string root*/
    json_root = cJSON_CreateObject();

    char rsp_type[30] = {0};
    snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

    if (!json_root) 
    {   
        cloudc_debug("%s[%d]: get json_root faild !", __func__, __LINE__);
        goto EXIT;
    }   
    else 
    {   
        cloudc_debug("%s[%d]: get json_root success!", __func__, __LINE__);
    }   

    {   
        cJSON_AddStringToObject(json_root, "type", rsp_type); 
        cJSON_AddNumberToObject(json_root, "commandId", serial); 
        cJSON_AddNumberToObject(json_root, "update", update_status); 
        cJSON_AddNumberToObject(json_root, "url", replace_status); 
        {   
            char *s = cJSON_PrintUnformatted(json_root);
            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }
    return 0;
EXIT:
    return -1;

}

int cloudc_build_rsp_get_js_buf(char *type, int serial, char *user_id, char *device_id, char *device_type, struct ipk_info *config_info_head, int key_name_num, char *js_buf) 
{
    cloudc_error("%s[%d]: Enter!", __func__, __LINE__);
    cJSON *json_root;
    struct ipk_info *tmp_config_info_head = config_info_head;

    /*create json string root*/
    json_root = cJSON_CreateObject();

    if (!json_root)
    {
        cloudc_error("%s[%d]: get json_root faild !\n", __func__, __LINE__);
        goto EXIT;
    }

    else
    {
        cloudc_debug("%s[%d]: get json_root succeed !\n", __func__, __LINE__);
    }

    {
        cJSON * js_array, *js_body ;

        char rsp_type[30] = {0};
        snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

        cJSON_AddStringToObject(json_root, "type", rsp_type);
        cJSON_AddNumberToObject(json_root, "commandId", serial);
        cJSON_AddStringToObject(json_root, "userId", user_id);
        cJSON_AddStringToObject(json_root, "deviceId", device_id);
        cJSON_AddStringToObject(json_root, "deviceType", device_type);
        cJSON_AddNumberToObject(json_root, "status", 2);
        cJSON_AddItemToObject(json_root, "config", js_array= cJSON_CreateArray());

        int i = 0;
        for (i = 0; i < key_name_num; i ++)
        {            
            cJSON_AddItemToArray(js_array, js_body = cJSON_CreateObject());
            cJSON_AddItemToObject(js_body, tmp_config_info_head->op_ipk_name, cJSON_CreateString(tmp_config_info_head->node_value));
            tmp_config_info_head = tmp_config_info_head->next;
        }

        {
            char *s = cJSON_PrintUnformatted(json_root);

            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }

    return 0;
EXIT:
    return -1;
}


int cloudc_build_rsp_set_js_buf(char *type, int serial, char *user_id, char *device_id, char *devData, int retCode, char *js_buf)
{
    cJSON *json_root;

    cloudc_debug("%s[%d]: Enter!", __func__, __LINE__);
    /*create json string root*/
    json_root = cJSON_CreateObject();

    if (!json_root)
    {
        cloudc_error("%s[%d]: get json_root faild !\n", __func__, __LINE__);
        goto EXIT;
    }
    else
    {
        cloudc_debug("%s[%d]: get json_root succeed !\n", __func__, __LINE__);
    }

    {
        cJSON * js_array, *js_body ;

        char rsp_type[30] = {0};
        snprintf(rsp_type, sizeof(rsp_type), "rsp_%s", type);

        cJSON_AddStringToObject(json_root, "type", rsp_type);
        cJSON_AddNumberToObject(json_root, "commandId", serial);
        cJSON_AddStringToObject(json_root, "userId", user_id);
        cJSON_AddStringToObject(json_root, "deviceId", device_id);
        cJSON_AddNumberToObject(json_root, "status", 2);
        cJSON_AddNumberToObject(json_root, "ret", retCode);
       
        cloudc_debug("%s[%d]: add fields!\n", __func__, __LINE__);
        if(0 == retCode)
        {
            //cJSON * pSubJson = NULL;
            cJSON * pSubDevData = NULL;
            //pSubJson = cJSON_CreateObject();
            pSubDevData = cJSON_Parse(devData);

            /*if(NULL == pSubJson)
            {
                return -1; 
            }*/

            cJSON_AddItemToObject(json_root, "devData", pSubDevData);
            cloudc_debug(" add devData field");
        }
        {
            char *s = cJSON_PrintUnformatted(json_root);

            if (s)
            {
                strncpy(js_buf, s, SEND_MAX_BUF_LEN - 1);
                cloudc_debug("%s[%d]: create js_buf  is %s\n", __func__, __LINE__, js_buf);
                free(s);
            }
        }
        cJSON_Delete(json_root);
    }

    return 0;
EXIT:
    return -1;
}
