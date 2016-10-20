/* This file is used to buid packet as defined and send to server.
 * */
#include "iotc.h"

/* TODO: the path should not be gateway */
#define HTTP_HEAD_FIX "POST /gateway HTTP/1.1\r\n\
User-Agent: iotc/1.0\r\n\
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

void iotc_genHostStr(char *server)
{
    g_hostHdrLen = snprintf(g_hostStr, sizeof(g_hostStr)-1, HTTP_HEAD_HOST, server);
    g_hostStr[MAX_HOST_STR_LEN-1] = '\0';
}

static int iotc_getHttpHeadLen()
{
    if(HEAD_HOST_LEN == -1) 
    {
        return -1;
    }

    return (HEAD_FIX_LEN+HEAD_HOST_LEN+HEAD_CON_LEN_LEN);
}

extern char g_devId[];
extern unsigned int g_cmdId;

static int iotc_addHttpHeader(char *buf, int conLen)
{
    strncpy(buf, HTTP_HEAD_FIX, HEAD_FIX_LEN);
    strncpy(buf+HEAD_FIX_LEN, g_hostStr, HEAD_HOST_LEN);
    snprintf(buf+HEAD_FIX_LEN+HEAD_HOST_LEN, HEAD_CON_LEN_LEN, HTTP_HEAD_CON_LEN, conLen);
    *(buf+HEAD_FIX_LEN+HEAD_HOST_LEN+HEAD_CON_LEN_LEN-1)='\n';

    return 0;
}

/* success: return http body length 
 * failure: return -1 */
static int iotc_buildDevOnlineData(char *buf, char *devData, int maxLen, unsigned int *cmdId)
{
    int ret=-1;
    cJSON *jsonRoot=NULL;
    cJSON *jsonDevData=NULL;
    char *s=NULL;

    iotc_debug("Enter.");

    iotc_debug("devData=%s", devData);
    if ((jsonDevData=cJSON_Parse(devData)) == NULL)
    {
        iotc_error("devData's format is wrong!");
        goto exit;
    }

    /*create json string root*/
    if ((jsonRoot=cJSON_CreateObject()) == NULL)
    {   
        iotc_error("create json root node failed!");
        goto exit1;
    }

    *cmdId = g_cmdId;

    cJSON_AddStringToObject(jsonRoot, "type", "device_online"); 
    cJSON_AddNumberToObject(jsonRoot, "commandId", g_cmdId++);


    cJSON_AddItemToObject(jsonRoot, "devData", jsonDevData);

    if ((s=cJSON_PrintUnformatted(jsonRoot)) == NULL)
    {
        iotc_error("convert json to string format failed!");
        goto exit2;
    }

    ret=snprintf(buf, maxLen, "%s", s);
    buf[maxLen-1] = '\0';
    iotc_debug("buf=%s\n", buf);
    free(s);

exit2:
    cJSON_Delete(jsonRoot);
    return ret;

exit1:
    cJSON_Delete(jsonDevData);
exit:
    iotc_debug("Exit.");
    return ret;
}

/* success: return http body length 
 * failure: return -1 */
static int iotc_buildSetRspData(char *buf, int maxLen, int cmdId, int retCode, cJSON *jsonDevData)
{
    int ret=-1;
    cJSON *jsonRoot=NULL;
    char *s=NULL;
    int needDetachJsonDevData=0;

    iotc_error("Enter.");

    /*create json string root*/
    if ((jsonRoot=cJSON_CreateObject()) == NULL)
    {
        iotc_error("create json root node failed!");
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
        iotc_error("convert json to string format failed!");
        goto exit1;
    }

    ret=snprintf(buf, maxLen, "%s", s);
    buf[maxLen-1] = '\0';
    iotc_debug("buf=%s\n", buf);
    free(s);

    if (needDetachJsonDevData == 1)
    {
        cJSON_DetachItemFromObject(jsonRoot, "devData");
    }

exit1:
    cJSON_Delete(jsonRoot);
exit:
    iotc_debug("Exit.");
    return ret;
}

static int iotc_sendHttpBuf(char *buf, int len)
{
    int ret=-1;

    iotc_debug("Enter.");

    if ( NULL==buf || len < 10 )
    {
        iotc_error("param error! len=%d", len);
        return -1;
    }

    if (-1 == is_socket_connected(iotc_monitor_uloop_fd.fd))
    {
        close(iotc_monitor_uloop_fd.fd);
        //socket_init();
    }

    ret=send(iotc_monitor_uloop_fd.fd, buf, len, 0);

    if (ret > 0)
    {
        iotc_debug("send http buf succeed!");
        return 0;
    }
    else if (ret == -1)
    {
        iotc_error("send msg failed! %s!", strerror(errno));
        close(iotc_monitor_uloop_fd.fd);
        //socket_create();
        //socket_connect();
        /* TODO: how to handle send fail? */
        //iotc_send_http_buf(buf);
    }
    else
    {
        iotc_error("send msg failed! ret=%d, %s.", ret, strerror(errno));
        return -1;
    }
    iotc_debug("Exit.");
}

int iotc_sendDevOnlineMsg()
{
    int ret=-1;
    int headLen=0, conLen=-1;
    int cmdId=0;
    char httpBuf[SEND_MAX_BUF_LEN]={0};
    char *devData=NULL;

    iotc_debug("Enter.");

    if ((devData=glue_getDevData()) == NULL)
    {
        iotc_error("failed to get devData!");
        return -1;
    }

    if ((headLen=iotc_getHttpHeadLen()) == -1)
    {
        iotc_error("iot server string is missing!");
        return -1;
    }

    iotc_debug("headLen=%d", headLen);

    /* reserve headLen bytes for http head */
    if ((conLen=iotc_buildDevOnlineData(&httpBuf[headLen], devData, SEND_MAX_BUF_LEN-headLen, &cmdId)) < 0)
    {
        iotc_error("failed to build http body!");
        return -1;
    }

    if ((ret=iotc_addHttpHeader(httpBuf, conLen)) != 0)
    {
        iotc_error("failed to build http head!");
        return -1;
    }

    iotc_debug("httpBuf=%s", httpBuf);

    if (iotc_sendHttpBuf(httpBuf, headLen+conLen) < 0)
    {
        iotc_error("send http pkt failed!");
        return -1;
    }

    if (iotcSess_waitRsp(MSG_DEV_ONLINE_RSP, cmdId) != 0)
    {
        iotc_error("malloc failed!");
        return -1;
    }

    iotc_debug("Exit.");

    return 0;
}

int iotc_sendSetRspMsg(int cmdId, int retCode, cJSON *jsonDevData)
{
    int ret=-1;
    int headLen=0, conLen=-1;
    char httpBuf[SEND_MAX_BUF_LEN]={0};

    iotc_debug("Enter.");

    if ((headLen=iotc_getHttpHeadLen()) == -1)
    {
        iotc_error("iot server string is missing!");
        return -1;
    }

    /* reserve headLen bytes for http head */
    if ((conLen=iotc_buildSetRspData(&httpBuf[headLen],
                                     SEND_MAX_BUF_LEN-headLen,
                                     cmdId,
                                     retCode,
                                     jsonDevData)) < 0)
    {
        iotc_error("failed to build http body!");
        return -1;
    }

    if ((ret=iotc_addHttpHeader(httpBuf, conLen)) != 0)
    {
        iotc_error("failed to build http head!");
        return -1;
    }

    iotc_debug("httpBuf=%s", httpBuf);

    iotc_sendHttpBuf(httpBuf, headLen+conLen);

    iotc_debug("Exit.");

    return ret;
}
