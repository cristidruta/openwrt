#include "iotc.h"

/* definition for msg type parser */
typedef struct {
    AgentMsgType type;
    char msgStr[64];
}AgentMsgMapping;

static AgentMsgMapping agentMsgArray[] = {
    {MSG_DEV_ONLINE_RSP, "rsp_device_online"},
    {MSG_SET, "set"},
    {MSG_UNKNOWN, "unknown"}
};

#define MAX_MSG_TYPE_COUNT (sizeof(agentMsgArray)/sizeof(AgentMsgMapping))

static AgentMsgType iotc_getMsgType(char *msgStr)
{
    int i=0;

    if(!msgStr)
    {
        return MSG_UNKNOWN;
    }

    for(i<0; i<MAX_MSG_TYPE_COUNT; i++) 
    {
        if(strcmp(msgStr, agentMsgArray[i].msgStr)==0)
            return agentMsgArray[i].type;
    }

    return MSG_UNKNOWN;
}
/* end of definition for msg type parser */


static int iotc_parseHttpHeader(char *recvbuf)
{
    iotc_debug("Enter.");
    /* TODO: */
    iotc_debug("Exit.");
    return 0;
}

static char* iotc_getHttpBody(char *recvbuf)
{
    char *body=NULL;

    iotc_debug("Enter.");
    body=strstr(recvbuf, "{");
    iotc_debug("Exit.");
    return body;
}

static int handle_devOnlineRsp(cJSON *json)
{
    cJSON *json_deviceId=NULL, *json_manufacture=NULL, *json_manufactureSN=NULL;
    cJSON *json_cmdId=NULL;

    iotc_debug("Enter.");

    json_deviceId = cJSON_GetObjectItem(json, "deviceId");
    json_cmdId = cJSON_GetObjectItem(json, "commandId");
    json_manufacture = cJSON_GetObjectItem(json, "manufacture");
    json_manufactureSN = cJSON_GetObjectItem(json, "manufactureSN");

    /* integrity and validity check */
    if(!json_deviceId || !json_manufacture || !json_manufactureSN || !json_cmdId)
    {
        iotc_error("missing json param!");
        return -1;
    }


    if (json_deviceId->type != cJSON_String ||
        json_manufacture->type != cJSON_String ||
        json_manufactureSN->type != cJSON_String ||
        json_cmdId->type != cJSON_Number )
    {
        iotc_error("param type is wrong!");
        return -1;
    }

    if (glue_isMyself(json_manufacture->valuestring, json_manufactureSN->valuestring) == 0)
    {
        iotc_error("got MSG_DEV_ONLINE_RSP not for myself! %s-%s",
                json_manufacture->valuestring, 
                json_manufactureSN->valuestring);
        return -1;
    }

    glue_updateDevId(json_deviceId->valuestring);

    if (iotcSess_gotRsp(MSG_DEV_ONLINE_RSP, json_cmdId->valueint) != 1)
    {
        iotc_error("got unexpected MSG_DEV_ONLINE_RSP, cmdId=%d", json_cmdId->valueint);
        return -1;
    }

    iotc_debug("Exit.");
    return 0;
}

static int handle_set(cJSON *json)
{
    cJSON *json_deviceId=NULL, *json_cmdId=NULL, *json_devData=NULL;

    iotc_debug("Enter.");

    json_deviceId = cJSON_GetObjectItem(json, "deviceId");
    json_cmdId = cJSON_GetObjectItem(json, "commandId");
    json_devData = cJSON_GetObjectItem(json, "devData");

    /* integrity and validity check */
    if(!json_deviceId || !json_cmdId || !json_devData)
    {
        iotc_error("missing json param!");
        return -1;
    }


    if (json_deviceId->type != cJSON_String ||
        json_cmdId->type != cJSON_Number ||
        json_devData->type != cJSON_Object )
    {
        iotc_error("param type is wrong!");
        return -1;
    }

    /* TODO: now always reply with success
     *       need add setting processing */
    glue_handleSet(json_devData);
    iotc_sendSetRspMsg(json_cmdId->valueint/*cmdId*/, 0/*retCode*/, json_devData);

    iotc_debug("Exit.");
    return 0;
}

int iotc_parseHttpBody(char *buf)
{
    AgentMsgType msgType;
    cJSON *json=NULL, *json_type=NULL;

    iotc_debug("Enter.");

    iotc_debug("len=%d \r\nbuf=%s", strlen(buf), buf);

    /* parse json_buf */
    if ((json=cJSON_Parse(buf)) == NULL)
    {
        //iotc_error("json parse http body failed! %s\n", cJSON_GetErrorPtr());
        iotc_error("json parse http body failed!\n");
        return -1;
    }


    /* parse item "type" */ 
    if ((json_type=cJSON_GetObjectItem(json, "type")) == NULL)
    {
        iotc_error("type is missing!");
        cJSON_Delete(json);  
        return -1;
    }

    if ((json_type->type != cJSON_String) || (NULL == json_type->valuestring))
    {
        iotc_error("type's value is missing or in wrong format!");
        cJSON_Delete(json);
        return -1;
    }

    iotc_debug("type=%s", json_type->valuestring);

    msgType=iotc_getMsgType(json_type->valuestring);
    switch(msgType)
    {
        case MSG_DEV_ONLINE_RSP:
            handle_devOnlineRsp(json);
            break;

        case MSG_SET:
            handle_set(json);
            break;

        defalut:
            iotc_error("Can not find msgType for %s\r\n", json_type->valuestring);
            break;
    }

    /* free */
    cJSON_Delete(json);

    iotc_debug("Exit.");
    return 0;
}

int iotc_handleServerPkt(char *recvbuf)
{
    char *body = NULL;

    iotc_debug("Enter.");
    if (0 == iotc_parseHttpHeader(recvbuf))
    {
        if ((body=iotc_getHttpBody(recvbuf)) != NULL)
        {
            iotc_parseHttpBody(body);
        }
    }

    iotc_debug("Exit.");
    return 0;
}
