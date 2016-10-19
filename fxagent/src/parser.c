#include "share.h"
#include "parser.h"
#include "cJSON.h"
#include <sqlite3.h>

int cloudc_parse_receive_info(char *recvbuf);
int cloudc_get_type(char *op_type);
int cloudc_get_serial_num(int serial_num);
int cloudc_parse_http_header(char *recvbuf);
char *cloudc_get_http_body(char *recvbuf);
int cloudc_parse_http_body(char *json_buf);
int cloudc_parse_alljoyn_notification(const char *json_buf);
void cloudc_get_ipk_name(struct ipk_info *list_head, char *ipk_name, char *node_value);
void cloudc_print_ipk_name_list(struct ipk_info *list_head);
void cloudc_destroy_ipk_name(struct ipk_info *list_head);
char *define_data_by_device_type(char *device_type);

http_value recvdata;
int rsp_status = 0;
char keyName[3][30] = {"power_switch", "status", "color_rgb"};

/* definition for msg type parser */
typedef enum {
    MSG_DEV_ONLINE_RSP=0,
    MSG_UNKNOWN
}AgentMsgType;

typedef struct {
    AgentMsgType type;
    char msgStr[64];
}AgentMsgMapping;

static AgentMsgMapping agentMsgArray[] = {
    {MSG_DEV_ONLINE_RSP, "rsp_device_online"},
    {MSG_UNKNOWN, "unknown"}
};

#define MAX_MSG_TYPE_COUNT (sizeof(agentMsgArray)/sizeof(AgentMsgMapping))

static AgentMsgType agent_getMsgType(char *msgStr)
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

int cloudc_parse_receive_info(char *recvbuf)
{
    char *http_body = NULL;
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    if (0 == cloudc_parse_http_header(recvbuf))
    {
        http_body = cloudc_get_http_body(recvbuf);

        if (NULL != http_body)
        {
            cloudc_parse_http_body(http_body);
        }
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_parse_http_header(char *recvbuf)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    /* need to parse http value firstly
     * such as return code
     * ...
     * if return code is ok, then return 0 and go on parse body value
     * */

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

char *cloudc_get_http_body(char *recvbuf)
{
    char *http_body = NULL;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    http_body = strstr(recvbuf, "{");

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return http_body;
}

static int handle_dev_online_rsp(cJSON *json)
{
    cJSON *json_deviceId, *json_manufacture, *json_manufactureSN;

    cloudc_debug("Enter.");

    json_deviceId = cJSON_GetObjectItem(json, "deviceId");
    json_manufacture = cJSON_GetObjectItem(json, "manufacture");
    json_manufactureSN = cJSON_GetObjectItem(json, "manufactureSN");

    /* integrity and validity check */
    if(!json_deviceId || !json_manufacture || !json_manufactureSN)
    {
        cloudc_error("missing json param!");
        return -1;
    }


    if (json_deviceId->type != cJSON_String ||
        json_manufacture->type != cJSON_String ||
        json_manufactureSN->type != cJSON_String )
    {
        cloudc_error("param type is wrong!");
        return -1;
    }

    /* TODO: is agent need store gatewayId ? */
#if 0
    if(strcmp(json_manufacture->valuestring, g_manufacture) == 0 &&
            strcmp(json_manufactureSN->valuestring, g_manufactureSN) == 0)
    {
        /* this response is for myself(gateway) */
        if(g_devId[0] == 0) 
        {
            if(strlen(json_deviceId->valuestring) > MAX_DEV_ID_LEN)
            {
                cloudc_error("deviceId %s is exceed mas len!\r\n", json_deviceId->valuestring);
                return -1;
            }
            strcpy(g_devId, json_deviceId->valuestring);
        }
        else
        {
            if(strcmp(g_devId, json_deviceId->valuestring))
            {
                cloudc_error("deviceId %s is unexpected(%d).\r\n", json_deviceId->valuestring, g_devId);
                /* TODO: how to handle this error */
                if(strlen(json_deviceId->valuestring) <= MAX_DEV_ID_LEN)
                {
                    strcpy(g_devId, json_deviceId->valuestring);
                }
            }
        }
    }
#endif

    /* this response is for other device */
    /* TODO: */
    http_value taskData;
    taskData.rpc_flag = eAlljoynUpdateDeviceId;
    strncpy(taskData.device_id, json_deviceId->valuestring, sizeof(taskData.device_id) - 1);
    //strncpy(taskData.device_sn, json_manufacture->valuestring, sizeof(taskData.device_sn) - 1);
    strncpy(taskData.device_sn, json_manufactureSN->valuestring, sizeof(taskData.device_sn) - 1);
    task_queue_enque(&queue_head, &taskData);

    cloudc_debug("Exit.");
    return 0;
}

int cloudc_parse_http_body(char *json_buf)
{
    struct ipk_info *head = NULL;
    int recv_status_count = 0;
    int i = 0;
    eOpType flag;

    char *array_item = NULL;
    char *array_item_value = NULL;
    char array_item_tmp[MAX_IPK_NAME_LEN] = {0};
    cJSON *json, *json_type, *json_serial, *json_status, *json_package, *json_update, *json_url, *json_user_id, *json_device_id, *json_action, *download_list, *delete_list;
    cJSON *json_deviceId, *json_deviceSn;
    cJSON *p_array_item = NULL;
    AgentMsgType msgType;

    cloudc_debug("%s[%d]: Enter.\r\n", __func__, __LINE__);
    cloudc_debug("%s[%d]: json_buf = %s\r\n", __func__, __LINE__, json_buf);

    /* parse json_buf */
    json = cJSON_Parse(json_buf);  

    if (!json)  
    {   
        cloudc_debug("%s[%d]: not json format ", __func__, __LINE__);
        cloudc_error("%s[%d]: Error before: [%s]\n", __func__, __LINE__, cJSON_GetErrorPtr());  
        return 0;
    }

    memset(&recvdata,0,sizeof(recvdata)); 
    /* parse item "type" */ 
    json_type = cJSON_GetObjectItem(json, "type");  

    if (NULL == json_type)
    {
        cloudc_debug("%s[%d]: failed to get type from json", __func__, __LINE__);
        cJSON_Delete(json);  
        return 0;
    }

    if ((json_type->type != cJSON_String) || (NULL == json_type->valuestring))
    {
        cloudc_error("%s[%d]: type value is null or not string");
        cJSON_Delete(json);
        return 0;
    }

    /* json_type->type: cJSON_String
     * json_type->valuestring is not NULL   */
    cloudc_debug("%s[%d]: type value = %s", __func__, __LINE__, json_type->valuestring);  

    recvdata.rpc_flag = cloudc_get_type(json_type->valuestring);
    recv_status_count++;
    cloudc_debug("%s[%d]: rpc_flag = %d", __func__, __LINE__, recvdata.rpc_flag);  

    /* TODO: need replace rpc.flag with msgType */
    msgType=agent_getMsgType(json_type->valuestring);
    switch(msgType)
    {
        case MSG_DEV_ONLINE_RSP:
            handle_dev_online_rsp(json);
            cJSON_Delete(json);
            return;

        defalut:
            cloudc_error("Can not find msgType for %s\r\n", json_type->valuestring);
    }

    json_serial = cJSON_GetObjectItem(json, "commandId");

    if (NULL == json_serial)
    {
        cloudc_debug("%s[%d]: failed to get serial from json", __func__, __LINE__);
    }
    else if (json_serial->type == cJSON_Number )   
    {  
        recvdata.serial_num = cloudc_get_serial_num(json_serial->valueint);
        recv_status_count++;
        cloudc_debug("%s[%d]: serial number = %d", __func__, __LINE__, json_serial->valueint);  
    }  
    else
    {
        cloudc_error("%s[%d]: serial value is not int", __func__, __LINE__);
    }

    switch(recvdata.rpc_flag)
    {
        case eRegister: /* need three key words: type, serial, status */

            json_status = cJSON_GetObjectItem(json, "status");  

            if (NULL == json_status)
            {
                cloudc_debug("%s[%d]: failed to get status from json", __func__, __LINE__);
            }
            else if (json_status->type == cJSON_Number )   
            {  
                recvdata.register_status = json_status->valueint;
                recv_status_count++;
                cloudc_debug("%s[%d]: register_status = %d", __func__, __LINE__, json_status->valueint);  
            } 
            else
            {
                cloudc_error("%s[%d]: status value is not int");
            }

            if (3 == recv_status_count)
            {
                rsp_status = 1;
                /* ap_register_flag = recvdata.register_status; */
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]:  rsp_status = %d, correct parameter, will continue to handle it", __func__, __LINE__, rsp_status);  

                task_queue_enque(&queue_head, &recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);  
            }

            break;

        case eQueryOperation: /* need two key words: type, serial */
            if (2 == recv_status_count)
            {
                rsp_status = 1;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]: correct parameter, will continue to handle it", __func__, __LINE__);  

                task_queue_enque(&queue_head, &recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);  
            }
            break;

        case eOpkgOperation: /* need four key words: type, serial, update, url */
            json_update = cJSON_GetObjectItem(json, "update");  
            if (NULL == json_update)
            {
                cloudc_debug("%s[%d]: failed to get update from json", __func__, __LINE__);
            }
            else if (json_update->type == cJSON_Number )   
            {  
                recvdata.opkg_update_flag = json_update->valueint;
                recv_status_count++;
                cloudc_debug("%s[%d]: opkg_update_flag = %d", __func__, __LINE__, json_update->valueint);  
            }
            else
            {
                cloudc_error("%s[%d]: update value is not int");
            }


            json_url = cJSON_GetObjectItem(json, "url");
            if (NULL == json_url)
            {
                cloudc_debug("%s[%d]: failed to get url from json", __func__, __LINE__);
            }
            else if (json_url->type == cJSON_String)
            {
                strncpy(recvdata.opkg_update_url, json_url->valuestring, sizeof(recvdata.opkg_update_url) - 1);
                recv_status_count++;
                cloudc_debug("%s[%d]: opkg_update_url = %s", __func__, __LINE__, json_url->valuestring);  
            }
            else
            {
                cloudc_error("%s[%d]: url value is not string");
            }


            if (4 == recv_status_count)
            {
                rsp_status = 1;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]: correct parameter, will continue to handle it", __func__, __LINE__);  

                task_queue_enque(&queue_head, &recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);  
            }
            break;

        case eServiceOperation: /* need three key words: type, serial, package */
            head = (struct ipk_info *)malloc(sizeof(struct ipk_info));
            if (NULL == head)
            {
                cloudc_error("%s[%d]: malloc failed", __func__, __LINE__);
                cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
                return;
            }

            memset(head, 0, sizeof(struct ipk_info));

            json_package = cJSON_GetObjectItem(json, "package");  
            if (NULL == json_package)
            {
                cloudc_debug("%s[%d]: failed to get package from json", __func__, __LINE__);
            }
            else if (json_package->type == cJSON_Array)
            {
                recvdata.real_ipk_num = cJSON_GetArraySize(json_package);

                if (0 < recvdata.real_ipk_num)
                {
                    recv_status_count++;

                    while (i < recvdata.real_ipk_num)
                    {
                        p_array_item = cJSON_GetArrayItem(json_package, i);
                        array_item  = cJSON_Print(p_array_item); 
                        array_item[strlen(array_item) - 1] = '\0';
                        strncpy(array_item_tmp, &array_item[1], MAX_IPK_NAME_LEN - 1);
                        cloudc_get_ipk_name(head, array_item_tmp, NULL);
                        i++;
                    }

                    recvdata.ipk_name_head = head;
                }
            }
            else
            {
                cloudc_error("%s[%d]: package value is not array");
            }

            if (3 == recv_status_count)
            {
                rsp_status = 1;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]: correct parameter, will continue to handle it", __func__, __LINE__);  

                task_queue_enque(&queue_head, &recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);  
            }

            break;

        case eIpkOperation: /* need three key words: type, serial, package */

            head = (struct ipk_info *)malloc(sizeof(struct ipk_info));

            if (NULL == head)
            {
                cloudc_error("%s[%d]: malloc failed", __func__, __LINE__);
                cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
                return;
            }

            memset(head, 0, sizeof(struct ipk_info));

            json_package = cJSON_GetObjectItem(json, "package");  

            if (NULL == json_package)
            {
                cloudc_debug("%s[%d]: failed to get package from json", __func__, __LINE__);
            }
            else if (json_package->type == cJSON_Array)
            {
                recvdata.real_ipk_num = cJSON_GetArraySize(json_package);
                if (0 < recvdata.real_ipk_num)
                {
                    recv_status_count++;
                    while (i < recvdata.real_ipk_num)
                    {
                        p_array_item = cJSON_GetArrayItem(json_package, i);
                        array_item  = cJSON_Print(p_array_item); 
                        array_item[strlen(array_item) - 1] = '\0';
                        strncpy(array_item_tmp, &array_item[1], MAX_IPK_NAME_LEN - 1);
                        cloudc_get_ipk_name(head, array_item_tmp, NULL);
                        i++;
                    }

                    recvdata.ipk_name_head = head;
                }
            }
            else
            {
                cloudc_error("%s[%d]: package value is not array");
            }


            if (3 == recv_status_count)
            {
                rsp_status = 1;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]: correct parameter, will continue to handle it", __func__, __LINE__);  
                task_queue_enque(&queue_head, &recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);  
            }
            break;

        case ePluginOperation:
            json_action = cJSON_GetObjectItem(json, "action");  
            if (NULL == json_action)
            {
                cloudc_debug("%s[%d]: failed to get plugin action from json", __func__, __LINE__);
            }
            else if (json_action->type == cJSON_String )   
            {  
                recvdata.plugin_action_flag = cloudc_get_action_type(json_action->valuestring);
                recv_status_count++;
                cloudc_debug("%s[%d]: plugin_action_flag = %d", __func__, __LINE__, recvdata.plugin_action_flag); 
            }
            else
            {
                cloudc_error("%s[%d]: update value is not int");
            }

            memset(recvdata.pluginUrl, 0, sizeof(MAX_PLUGIN_URL_LIST_LEN));
            memset(recvdata.pluginDeleteList, 0, sizeof(MAX_PLUGIN_URL_LIST_LEN));

            if (eDelete == recvdata.plugin_action_flag)
            {
                delete_list = cJSON_GetObjectItem(json, "pluginDeleteList");

                if (NULL == delete_list)
                {
                    cloudc_debug("%s[%d]: no delete plugin from json", __func__, __LINE__);
                }
                else if (delete_list->type == cJSON_Array)
                {
                    recvdata.real_ipk_num = cJSON_GetArraySize(delete_list);
                    if (0 < recvdata.real_ipk_num)
                    {
                        recv_status_count++;
                        while (i < recvdata.real_ipk_num)
                        {
                            p_array_item = cJSON_GetArrayItem(delete_list, i);
                            array_item  = cJSON_Print(p_array_item); 

                            strncat(recvdata.pluginDeleteList, &array_item[1], strlen(array_item)-2);
                            strcat(recvdata.pluginDeleteList, ",");

                            i++;
                        }

                        strcat(recvdata.pluginDeleteList, "\0");

                    }
                    cloudc_debug("%s[%d]: pluginDeleteList=%s", __func__,__LINE__,recvdata.pluginDeleteList);

                }
                else
                {
                    cloudc_error("%s[%d]: wrong plugin list", __func__, __LINE__);
                }

            }
            else
            {
                download_list = cJSON_GetObjectItem(json, "pluginDownloadList");

                if (NULL == download_list)
                {
                    cloudc_debug("%s[%d]: no download url from json", __func__, __LINE__);
                }
                else if (download_list->type == cJSON_Array)
                {
                    recvdata.real_ipk_num = cJSON_GetArraySize(download_list);
                    if (0 < recvdata.real_ipk_num)
                    {
                        recv_status_count++;
                        while (i < recvdata.real_ipk_num)
                        {
                            p_array_item = cJSON_GetArrayItem(download_list, i);
                            array_item  = cJSON_Print(p_array_item); 

                            strncat(recvdata.pluginUrl, &array_item[1], strlen(array_item)-2);
                            strcat(recvdata.pluginUrl, ",");

                            i++;
                        }

                        strcat(recvdata.pluginUrl, "\0");

                    }
                    cloudc_debug("%s[%d]: pluginUrl=%s", __func__,__LINE__,recvdata.pluginUrl);

                }
                else
                {
                    cloudc_error("%s[%d]: wrong plugin list", __func__, __LINE__);
                }

            }

            
            if (4 == recv_status_count)
            {
                rsp_status = 1;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_debug("%s[%d]: correct parameter, will continue to handle it", __func__, __LINE__);  

                task_queue_enque(&queue_head, &recvdata);

            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                cloudc_error("%s[%d]: wrong parameter, no need to handle it", __func__, __LINE__);
            }

            break;

        case eAlljoynGetOperation:
            break;

        case eAlljoynSetOperation:
            cloudc_debug("eAlljoynSetOperation");
            json_user_id = cJSON_GetObjectItem(json, "userId");
            if (NULL == json_user_id)
            {
                cloudc_debug("Failed to get user_id from json!");
            }
            else if (json_user_id->type == cJSON_String)
            {
                strncpy(recvdata.user_id, json_user_id->valuestring, MAX_USERID_LEN);
                recv_status_count++;
                cloudc_debug("userId=%s", recvdata.user_id);
            }
            else
            {
                cloudc_error("userId value is not string");
            }

            json_device_id = cJSON_GetObjectItem(json, "deviceId");
            if (NULL == json_device_id)
            {
                cloudc_debug("failed to get deviceId from json");
            }
            else if (json_device_id->type == cJSON_String)
            {
                strncpy(recvdata.device_id, json_device_id->valuestring, MAX_DEVICEID_LEN);
                recv_status_count++;
                cloudc_debug("deviceId=%s", recvdata.device_id);  
            }
            else
            {
                cloudc_error("deviceId value is not string!");
            }

            char manufacture[MAX_MANUFACTURE_LEN] = {0};
            char moduleNumber[MAX_MODULENUMBER_LEN] = {0};
            cJSON *json_keyNode = cJSON_GetObjectItem(json, "devData");
            if(json_keyNode == NULL)
            {
                cloudc_debug("Cannot find devData key in config!");
            }
            else
            {
                char *s = cJSON_PrintUnformatted(json_keyNode);
                if(s)
                {
                    recv_status_count++;
                    recvdata.devData = (char *)malloc(MAX_DEVDATA_LEN);
                    memset(recvdata.devData, 0, MAX_DEVDATA_LEN);
                    strncpy(recvdata.devData, s, MAX_DEVDATA_LEN - 1);     
                    cloudc_debug("recvdata.devData = %s", recvdata.devData);
                    free(s);
                }
            }

            if (5 == recv_status_count)
            {
                rsp_status = 1;
                //cloudc_send_alljoyn_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, recvdata.user_id, recvdata.device_id, rsp_status);
                cloudc_debug("correct parameter, add to task queue");

                //task_queue_enque(&queue_head, &recvdata);
                cloudc_rpc_method_handle(recvdata);
            }
            else
            {
                rsp_status = 0;
                cloudc_send_alljoyn_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, recvdata.user_id, recvdata.device_id, rsp_status);
                cloudc_error("wrong param!");
            }
            break;

        case eAlljoynUpdateDeviceId:
            cloudc_debug("%s[%d]: This is eAlljoynUpdateDeviceId flag", __func__, __LINE__);
            json_deviceId = cJSON_GetObjectItem(json, "deviceId");  
            if (NULL == json_deviceId)
            {
                cloudc_debug("%s[%d]: failed to get deviceId from json", __func__, __LINE__);
            }
            else if (json_deviceId->type == cJSON_String )   
            {  
                recv_status_count++;
                strncpy(recvdata.device_id, json_deviceId->valuestring, sizeof(recvdata.device_id) - 1);
                cloudc_debug("%s[%d]: recvdata.device_id = %s", __func__, __LINE__, json_deviceId->valuestring);  
            }
            else
            {
                cloudc_error("%s[%d]: recvdata.device_id value is not string", __func__, __LINE__);
            }

            json_deviceSn = cJSON_GetObjectItem(json, "manufactureSN");  
            if (NULL == json_deviceSn)
            {
                cloudc_debug("%s[%d]: failed to get deviceSn from json", __func__, __LINE__);
            }
            else if (json_deviceSn->type == cJSON_String )   
            {  
                recv_status_count++;
                strncpy(recvdata.device_sn, json_deviceSn->valuestring, sizeof(recvdata.device_sn) - 1);
                cloudc_debug("%s[%d]: recvdata.device_sn = %s", __func__, __LINE__, json_deviceSn->valuestring);  
            }
            else
            {
                cloudc_error("%s[%d]: recvdata.device_sn value is not string", __func__, __LINE__);
            }


            if(4 == recv_status_count)
            {
                rsp_status = 1;
                /* 1. agent recive rsp_device_online buf succeed, need to respone server?
                 * 2. agent update deviceId succeed, need to response server?
                 * */
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
                task_queue_enque(&queue_head, &recvdata);
                /* need to send to server */
            }
            else
            {
                rsp_status = 0;
                cloudc_send_recv_rsp_buf(recvdata.rpc_cmd, recvdata.serial_num, rsp_status);
            }

            break;

        default:
            cloudc_error("%s[%d]: unknown flag, recvdata.rpc_cmd = %s, recvdata.rpc_flag = %d", __func__, __LINE__, recvdata.rpc_cmd, recvdata.rpc_flag);
            break;
    }

    /* free */
    cJSON_Delete(json);  

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

int cloudc_parse_alljoyn_notification(const char *json_buf)
{
    cJSON *json;

    cloudc_debug("%s[%d]: Enter ", __func__, __LINE__);
    cloudc_debug("%s[%d]: json_buf = %s", __func__, __LINE__, json_buf);

    /* parse json_buf */
    json = cJSON_Parse(json_buf);  

    if (!json)  
    {   
        cloudc_debug("%s[%d]: not json format, no need to continue ", __func__, __LINE__);
        cloudc_error("%s[%d]: Error before: [%s]\n", __func__, __LINE__, cJSON_GetErrorPtr());  
    }   
    else
    {
        cloudc_debug("%s[%d]: can go on parsing, will perfect later ", __func__, __LINE__);
    }

    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return 0;
}

int cloudc_get_type(char *op_type)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    if (NULL == op_type)
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return -1;
    }

    strncpy(recvdata.rpc_cmd, op_type, sizeof(recvdata.rpc_cmd) - 1);

    if (!strcmp(op_type, CMD_RSP_REGISTER))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eRegister;
    }

    else if (!strcmp(op_type, CMD_GET_INSTALL_IPK) || !strcmp(op_type, CMD_GET_RUNNING_IPK))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eQueryOperation;
    }

    else if (!strcmp(op_type, CMD_OPKG_UPDATE))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eOpkgOperation;
    }

    else if (!strcmp(op_type, CMD_START_IPK_SERVICE) || !strcmp(op_type, CMD_STOP_IPK_SERVICE) 
            || !strcmp(op_type, CMD_ENABLE_IPK_SERVICE) || !strcmp(op_type, CMD_DISABLE_IPK_SERVICE))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eServiceOperation;
    }

    else if (!strcmp(op_type, CMD_INSTALL_IPK) || !strcmp(op_type, CMD_UPGRADE_IPK) 
            || !strcmp(op_type, CMD_UNINSTALL_IPK))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eIpkOperation;
    }

    else if (!strcmp(op_type, CMD_PLUGIN_ACTION))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return ePluginOperation;
    }

    else if (!strcmp(op_type, CMD_ALLJOYN_GET_OPERATION))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eAlljoynGetOperation;
    }

    else if (!strcmp(op_type, CMD_ALLJOYN_SET_OPERATION))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eAlljoynSetOperation;
    }

    else if (!strcmp(op_type, CMD_ALLJOYN_UPDATE_DEVICEID))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eAlljoynUpdateDeviceId;
    }
    else
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eInvalidParam;
    }
}

int cloudc_get_action_type(char *action_type)
{
    if (NULL == action_type)
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return -1;
    }

    if (!strcmp(action_type, "install"))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eInstall;
    }
    else if (!strcmp(action_type, "update"))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eUpdate;
    }
    else if (!strcmp(action_type, "delete"))
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eDelete;
    }
    else
    {
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return eInvalidActionParam;
    }

}

int cloudc_get_serial_num(int serial_num)
{
    return serial_num;
}

void cloudc_get_ipk_name(struct ipk_info *list_head, char *ipk_name, char *node_value)
{
    struct ipk_info *ptr_new = NULL;
    struct ipk_info *ptr_tmp = NULL;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    cloudc_debug("%s[%d]: ipk_name = %s, node_value = %s\n", __func__, __LINE__, ipk_name, node_value);

    ptr_new = (struct ipk_info *)malloc(sizeof(struct ipk_info));

    if (NULL == ptr_new)
    {
        cloudc_error("%s[%d]: malloc failed", __func__, __LINE__);
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return;
    }

    memset(ptr_new, 0, sizeof(struct ipk_info));

    strncpy( ptr_new->op_ipk_name, ipk_name, MAX_IPK_NAME_LEN - 1);

    if(node_value != NULL)
    {
        strncpy( ptr_new->node_value, node_value, MAX_NODE_VALUE_LEN - 1);
    }

    cloudc_debug("%s[%d]: ptr_new->op_ipk_name = %s, ptr_new->node_value = %s", __func__, __LINE__, ptr_new->op_ipk_name, ptr_new->node_value);

#if 0 /* insert in head */
    if ( NULL == list_head)
    {
        list_head = ptr_new;
        list_head->next = NULL;
    }
    else
    {
        ptr_new->next = list_head;
        list_head = ptr_new;
    }
#endif

    /* insert form tail */
    if ( NULL == list_head)
    {
        list_head = ptr_new;
        list_head->next = NULL;
    }
    else
    {
        ptr_tmp = list_head;

        while( NULL != ptr_tmp->next)
        {
            ptr_tmp = ptr_tmp->next;
        }

        ptr_tmp->next = ptr_new;
        ptr_new->next = NULL;
    }
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return;
}

void cloudc_print_ipk_name_list(struct ipk_info *list_head)
{
    struct ipk_info *ptr;
    int j = 0;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    if(NULL == list_head)
    {
        cloudc_debug("%s[%d]: no record", __func__, __LINE__);
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return;
    }
    else
    {
        ptr = list_head;
        cloudc_debug("%s[%d]: info as below: \n", __func__, __LINE__);

        while( NULL != ptr)
        {
            cloudc_debug("%s[%d]: ipk name[%d] = %s", __func__, __LINE__, j, ptr->op_ipk_name);
            j = j + 1;
            ptr = ptr->next;
        }
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return;
    }
}

void cloudc_destroy_ipk_name(struct ipk_info *list_head)
{
    struct ipk_info *ptr_tmp1;
    struct ipk_info *ptr_tmp2;

    struct ipk_info *tmp_head = list_head;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    if( NULL == tmp_head)
    {
        cloudc_debug("%s[%d]no node to delete", __func__, __LINE__);
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
        return;
    }

    while( NULL != tmp_head->next)
    {
        ptr_tmp1 = tmp_head->next;
        tmp_head->next = ptr_tmp1->next;
        free(ptr_tmp1);
    }
    free(tmp_head);
    tmp_head = NULL;


    cloudc_debug("%s[%d]: Exit ", __func__, __LINE__);
    return;
}

char *define_data_by_device_type(char *device_type)
{
    if(strcmp(device_type, "led") == 0)
    {
        //define the data here
    }
    return NULL;
}

