#include "share.h"
#include "proto.h"
#include "parser.h"
#include "handle_sys_feature.h"
#include <curl/curl.h>
#define CLOUDC_GET_APMAC "/etc/config/network"
#include "handle_sys_feature.h"
#define MAX_PATH_LEN 256




static size_t my_fwrite(void *buffer, size_t size, size_t nmemb,
        void *stream)
{
    struct FtpFile *out=(struct FtpFile *)stream;
    if(out && !out->stream) {
        /* open file for writing */
        out->stream=fopen(out->filename, "wb");
        if(!out->stream)
            return -1; /* failure, can't open file to write */
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

int get_ap_info(void)
{
    /* need to get ap_mac and ap_sn here
     * for ap_mac: can get from /etc/config/network
     * for ap_sn: not sure yet...
     * */
    char temp_ap_mac[MAX_MAC_LEN] = "42:21:33:dd:08:08";
    char temp_ap_sn[MAX_SN_LEN] = "tempserialnumber0099";
    strncpy(ap_mac, temp_ap_mac, sizeof(ap_mac) - 1);
    strncpy(ap_sn, temp_ap_sn, sizeof(ap_mac) - 1);
    return 0;
}

char* get_plugin_name(char *url)
{
    char *tmp = strdup(url);
    char *p, *p_underline;
    char name[32];

    while((p = strsep(&tmp, "/")) != NULL)
    {
        if (NULL != strstr(p, ".ipk"))
        {
            cloudc_debug("%s[%d]: plugin_name=%s", __func__, __LINE__, p);
            if ((p_underline = strchr(p, '_')) != NULL)
            {
                strncpy(name, p, p_underline-p);
                name[p_underline-p] = '\0';
            }
            else
            {
                strncpy(name, p, strlen(p)-4);
                name[strlen(p)-4] = '\0';
            }
        }
    }

    free(tmp);

    return strdup(name);
}

char* get_file_name(char *ipk)
{
    char *tmp = strdup(ipk);
    char *p_underline;
    char name[32];

    if (NULL != strstr(tmp, ".ipk"))
    {
        if ((p_underline = strchr(tmp, '_')) != NULL)
        {
            strncpy(name, tmp, p_underline-tmp);
            name[p_underline-tmp] = '\0';
        }
        else
        {
            strncpy(name, tmp, strlen(tmp)-4);
            name[strlen(tmp)-4] = '\0';
        }
    }

    free(tmp);
    return strdup(name);
}

void cloudc_rpc_method_handle(struct http_value recv_data)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    switch(recv_data.rpc_flag)
    {
        case eQueryOperation:
            cloudc_manage_query(recv_data.rpc_cmd, recv_data.serial_num);
            break;
        //add by whzhe
        case eDevUpgrade:
            cloudc_manage_dev_upgrade(recv_data.rpc_cmd, recv_data.serial_num, recv_data.device_id, recv_data.user_id, recv_data.dev_upgrade_url, recv_data.MD5);
            break;
        //end
        case eIpkOperation:
            cloudc_manage_ipk(recv_data.rpc_cmd, recv_data.serial_num, recv_data.ipk_name_head->next, recv_data.real_ipk_num);
            break;

        case eOpkgOperation:
            cloudc_manage_opkg(recv_data.rpc_cmd, recv_data.serial_num, recv_data.opkg_update_flag, recv_data.opkg_update_url);
            break;

        case ePluginOperation:
            cloudc_manage_plugin(recv_data.rpc_cmd, recv_data.serial_num, recv_data.plugin_head, recv_data.plugin_action_flag, recv_data.device_id);
            break;

        case eServiceOperation:
            cloudc_manage_service(recv_data.rpc_cmd, recv_data.serial_num, recv_data.ipk_name_head->next, recv_data.real_ipk_num);
            break;

        case eRegister:
            ap_register_flag = recv_data.register_status;
            
            cloudc_debug("%s[%d]: ap_register_flag = %d", __func__, __LINE__, ap_register_flag);
#if 0
            if (1 == recv_data.register_status)
            {
                cloudc_debug("%s[%d]: registered failed", __func__, __LINE__);
                usleep(10 * 1000);
                /* status = 1 means register fail
                 * need to send register request again
                 * */
                ap_register();
            }
            else if ((0 == recv_data.register_status) || (2 == recv_data.register_status))
            {
                cloudc_debug("%s[%d]: already registered", __func__, __LINE__);
                /* status = 0 means register succeed
                 * status = 2 meand already registered
                 * */
            }
#endif
            break;

        case eAlljoynGetOperation:
            break;

        case eAlljoynSetOperation:
            cloudc_manage_alljoyn_set_operation(recv_data.rpc_cmd, recv_data.serial_num, recv_data.user_id, recv_data.device_id, recv_data.device_type, recv_data.devData);
            break;

        case eAlljoynUpdateDeviceId:
            cloudc_manage_alljoyn_update_devid(recv_data.rpc_cmd, recv_data.device_id, recv_data.device_sn);
            cloudc_manage_alljoyn_set_devData(recv_data.device_id);
            break;

        default:
            cloudc_error("%s[%d]: unknow flag, rpc_flag = %d", __func__, __LINE__, recv_data.rpc_flag);
            break;
    }
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
}

int download_check(char *md5)
{
    /* need to compute the md5 of download file
     * and compare with the md5 which the server provide .
     * strcmp();
     *

    */
    return 0;
}

int cloudc_manage_dev_upgrade(char *op_type, int serial_num, char* device_id, char *user_id, char *dev_upgrade_url, char *MD5)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    int upgrade_status = -1;
    int download_status = -1;
    int check_status = -1;
    char ImpPath[MAX_PATH_LEN] = {0};

    if (NULL != *dev_upgrade_url)
    {
        download_status = cloudc_download_dev_file(dev_upgrade_url, ImpPath);
    
        cloudc_debug("ImpPath = %s", ImpPath);
    }
    else
    {
        download_status = 1;
        cloudc_debug("%s[%d]: EXIT: URL is NULL!", __func__, __LINE__);
        return 1;
    }
    if (download_status == 1)
    {
        cloudc_debug("%s[%d]: EXIT: download file failed!", __func__, __LINE__);
        return 1;
    }
    check_status = download_check(MD5);
    if (check_status == 0)
    {
        NotificationSendMain(op_type, serial_num, device_id, user_id, ImpPath);
    }
    else
    {
        cloudc_debug("%s[%d] download file is not complete!");
    }

//    cloudc_send_rsp_opkg_buf(op_type, serial_num, update_status, replace_status);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}



    
int cloudc_manage_opkg(char *op_type, int serial_num, int update_flag, char *url)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    int update_status = -1;
    int replace_status = -1;

    if (NULL != url)
    {
        /*
         * 1.download new opkg_conf from this url
         * 2.replace opkg.conf
         * */
        replace_status = cloudc_download_file(url);

        /* 0 means succeed, 1 means failed */
    }
    else
    {
        replace_status = 0; /* no need replace, so set it as 0 */
        cloudc_debug("%s[%d]: no need to replace opkg_conf file", __func__, __LINE__);
    }

    if (1 == update_flag)
    {
        /* need update */
        /* 0 means succeed, 1 means failed */
        update_status = cloudc_opkg_conf_update();
    }
    else
    {
        update_status = 0; /* no need update, so set it as 0 */
        cloudc_debug("%s[%d]: no need to update", __func__, __LINE__);
    }

    cloudc_send_rsp_opkg_buf(op_type, serial_num, update_status, replace_status);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_manage_plugin(char *op_type, int serial_num, struct plugin_info *head, int action_flag, char *deviceId)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    int status = -1;
    struct plugin_info *node = head;
    char *action = NULL;
    int action_status = 0;

    while(head)
    {
        node = head; 

        if (eInstall == action_flag)
        {
            char *name = get_plugin_name(node->url);

            cloudc_download_plugin(node->url);
            status = cloudc_install_plugin(name);
            free(name);
            
            action = strdup("install");
            action_status = status;
        }
        else if (eUpdate == action_flag)
        {
            char *name = get_plugin_name(node->url);

            cloudc_download_plugin(node->url);
            status = cloudc_remove_plugin(name);
            status = cloudc_install_plugin(name);
            free(name);

            action = strdup("update");
            action_status = status;
        }
        else if (eDelete == action_flag)
        {
            char *name = get_file_name(node->name);

            status = cloudc_remove_plugin(name);
            free(name);

            action = strdup("delete");
            action_status = status ? 0 : 1;
        }

        cloudc_debug("action=%s,ret=%d,deviceId=%s,pluginId=%d,version=%s",action,action_status,deviceId,node->pluginId,node->version);
        cloudc_send_rsp_plugin_buf(op_type, serial_num, action, action_status, deviceId, node->pluginId, node->version);

        head = node->next;
        free(node);
        free(action);
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_manage_query(char *op_type, int serial_num)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    if (0 == strcmp(op_type, CMD_GET_INSTALL_IPK))
    {
        cloudc_get_install_ipk_list(op_type, serial_num);
    }

    else if (0 == strcmp(op_type, CMD_GET_RUNNING_IPK))
    {
        cloudc_get_running_ipk_list(op_type, serial_num);
    }
    else
    {
        cloudc_debug("%s[%d]: unknown type", __func__, __LINE__);
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}

int cloudc_manage_ipk(char *op_type, int serial_num, struct ipk_info *ipk_list_head, int real_ipk_num)
{
    int status[real_ipk_num];
    int i = 0;
    struct ipk_info *ipk_list_tmp = ipk_list_head;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    memset(status, -1, sizeof(status));

    if (0 == strcmp(op_type, CMD_INSTALL_IPK))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_install_ipk(ipk_list_tmp->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to install %s", __func__, __LINE__, ipk_list_tmp->op_ipk_name);
            }

            ipk_list_tmp = ipk_list_tmp->next;
        }

        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else if (0 == strcmp(op_type, CMD_UNINSTALL_IPK))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_uninstall_ipk(ipk_list_tmp->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to uninstall %s", __func__, __LINE__, ipk_list_tmp->op_ipk_name);
            }

            ipk_list_tmp = ipk_list_tmp->next;
        }

        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else if (0 == strcmp(op_type, CMD_UPGRADE_IPK))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_upgrade_ipk(ipk_list_tmp->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to upgrade %s", __func__, __LINE__, ipk_list_tmp->op_ipk_name);
            }

            ipk_list_tmp = ipk_list_tmp->next;
        }
        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else
    {
        cloudc_error("%s[%d]: invaild op_type, need server confirm", __func__, __LINE__);
    }
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}

int cloudc_manage_service(char *op_type, int serial_num,  struct ipk_info *ipk_list_head, int real_ipk_num)
{
    int status[real_ipk_num];
    int i = 0;
    struct ipk_info *ipk_list_tmp = ipk_list_head;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    memset(status, -1, sizeof(status));

    if (0 == strcmp(op_type, CMD_START_IPK_SERVICE))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_start_ipk(ipk_list_tmp->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to start %s service", __func__, __LINE__, ipk_list_head->op_ipk_name);
            }

            ipk_list_tmp = ipk_list_tmp->next;

        }
        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else if (0 == strcmp(op_type, CMD_STOP_IPK_SERVICE))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_stop_ipk(ipk_list_head->op_ipk_name);
            {
                cloudc_error("%s[%d]: failed to start %s service", __func__, __LINE__, ipk_list_head->op_ipk_name);
            }

            ipk_list_tmp = ipk_list_tmp->next;
        }
        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else if (0 == strcmp(op_type, CMD_ENABLE_IPK_SERVICE))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_enable_ipk(ipk_list_head->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to enable %s service", __func__, __LINE__, ipk_list_head->op_ipk_name);
            }

        }
        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else if (0 == strcmp(op_type, CMD_DISABLE_IPK_SERVICE))
    {
        for (i = 0; i < real_ipk_num; i ++)
        {
            status[i] = cloudc_disable_ipk(ipk_list_head->op_ipk_name);
            if (-1 == status[i])
            {
                cloudc_error("%s[%d]: failed to disable %s service", __func__, __LINE__, ipk_list_head->op_ipk_name);
            }
        }
        cloudc_send_rsp_ipk_buf(op_type, serial_num, ipk_list_head, status, real_ipk_num);
    }

    else
    {
        cloudc_error("%s[%d]: invaild op_type, need server confirm", __func__, __LINE__);
    }
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}

int cloudc_manage_alljoyn_set_operation(char *op_type, int serial_num, char *user_id, char *device_id, char *device_type, char *devData)
{
    char objectPath[MAX_OBJECTPATH_LEN] = {0};
    char interfaceName[MAX_INTERFACE_LEN] = {0};
    char onlineStatus[MAX_ONLINESTATUS_LEN] = {0};
    char deviceType[MAX_DEVICETYPE_LEN] = {0};
    char manufacture[MAX_MANUFACTURE_LEN] = {0};
    char moduleNumber[MAX_MODULENUMBER_LEN] = {0};
    int msg_type = -1;
    int ret = 0;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);


    if(strcmp(op_type, "get") == 0)
    {
        msg_type = 0;
    }
    else if(strcmp(op_type, "set") == 0)
    {
        msg_type = 1;
    }

    cloudc_debug("%s[%d]: op_type = %s, serial_num = %d, user_id = %s, device_id = %s, device_type = %s\n", 
            __func__, __LINE__, op_type, serial_num, user_id, device_id, device_type);

    getObjInfoByDevId(device_id, interfaceName, objectPath, onlineStatus);
    if(strcmp(onlineStatus, "on") == 0)
    {
        if(NULL != devData)
        {
            ret = configClientMain(interfaceName,objectPath, msg_type, devData);
            cloudc_send_rsp_set_buf(op_type, serial_num, user_id, device_id, devData, ret);
        }
    }
    else
    {
        cloudc_debug("%s[%d]: device is already offline, cannot be configured", __func__, __LINE__);
        /* to do: need to send smartDevice offline event to server later ? */
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_manage_alljoyn_update_devid(char *op_type, char *device_id, char *device_sn)
{
    int ret = -1;
    ret = setDevIdByDevSn(device_id, device_sn);
    if(0 == ret)
    {
        cloudc_debug("%s[%d]:update deviceId succeed, need to send to server? \n", __func__, __LINE__);
    }
    else
    {
        cloudc_debug("%s[%d]:update deviceId failed, need to send to server? \n", __func__, __LINE__);
    }
}

int cloudc_manage_alljoyn_set_devData(char *deviceId)
{
    char objectPath[MAX_OBJECTPATH_LEN] = {0};
    char interfaceName[MAX_INTERFACE_LEN] = {0};
    char onlineStatus[MAX_ONLINESTATUS_LEN] = {0};
    char devData[MAX_DEVDATA_LEN] = {0};
    int msg_type = -1;
    int ret = 0;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    msg_type = 1; //for set


    getObjInfoByDevId(deviceId, interfaceName, objectPath, onlineStatus);
    getDevDataByDevId(deviceId, devData);
    cloudc_debug("%s[%d]: device_id = %s, devData = %s\n", __func__, __LINE__, deviceId, devData);

    if(strcmp(onlineStatus, "on") == 0)
    {
        if(NULL != devData)
        {
            configClientMain(interfaceName,objectPath, msg_type, devData, NULL, 0);
        }
    }
    else
    {
        cloudc_debug("%s[%d]: device is already offline, cannot be configured", __func__, __LINE__);
        /* to do: need to send smartDevice offline event to server later ? */
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}
int cloudc_download_dev_file(char *download_url, char *path)
{
    CURL *curl;
    CURLcode res;
    int download_status = -1;
    struct FtpFile ftpfile={
    "/var/fw/dev_opkg.temp", /* name to store the file as if succesful */
    NULL
    };
    char temp_buffer[128];

    memset(temp_buffer, 0, 128);

    strcpy(temp_buffer, "mkdir /var/fw");
    system(temp_buffer);
    
    cloudc_debug("%s[%d]: Enter, and url is %s", __func__, __LINE__, download_url);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        /*
        ** You better replace the URL with one that works! Note that we use an
        ** FTP:// URL with standard explicit FTPS. You can also do FTPS:// URLs if
        ** you want to do the rarer kind of transfers: implicit.
        **/
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        /* Switch on full protocol/debug output */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        /* always cleanup */
        
        curl_easy_cleanup(curl);

        if(CURLE_OK != res) {
                                        
            /* we failed */
                                      
            cloudc_debug("%s[%d]: curl told us %d\n", __func__, __LINE__, res);
            download_status = 1;            
        }
    }
    
    if(ftpfile.stream)        
        fclose(ftpfile.stream); /* close the local file */
   
    curl_global_cleanup();              
    if(1 == download_status)                                     
    {
        cloudc_debug("%s[%d]: Exit,download dev_opkg conf failure", __func__, __LINE__);                       
        return 1;
    }                
    else             
    {          
        if(0 == rename("/var/fw/dev_opkg.temp","/var/fw/dev_opkg.bin") )
        {
            cloudc_debug("%s[%d]: Exit,rename success", __func__, __LINE__);
            char *tempath = "http://172.17.60.162/fw/dev_opkg.bin";
            strncpy(path, tempath, MAX_PATH_LEN - 1);
              
            cloudc_debug("path = %s", path);
            return 0;
        }
        else
        {
            cloudc_debug("%s[%d]: Exit,rename failure", __func__, __LINE__);
            return 1;
        }
    }
}
int cloudc_download_file(char *download_url)
{
    CURL *curl;
    CURLcode res;
    int download_status = -1;
    struct FtpFile ftpfile={
        "/etc/opkg.temp", /* name to store the file as if succesful */
        NULL
    };

    cloudc_debug("%s[%d]: Enter, and url is %s", __func__, __LINE__, download_url);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        /*
         * You better replace the URL with one that works! Note that we use an
         * FTP:// URL with standard explicit FTPS. You can also do FTPS:// URLs if
         * you want to do the rarer kind of transfers: implicit.
         */
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        /* Switch on full protocol/debug output */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        /* always cleanup */
        curl_easy_cleanup(curl);

        if(CURLE_OK != res) {
            /* we failed */
            cloudc_debug("%s[%d]: curl told us %d\n", __func__, __LINE__, res);
            download_status = 1;
        }
    }

    if(ftpfile.stream)
        fclose(ftpfile.stream); /* close the local file */

    curl_global_cleanup();

    if(1 == download_status)
    {
        cloudc_debug("%s[%d]: Exit,download opkg conf failure", __func__, __LINE__);
        return 1;
    }
    else
    {
        if(0 == rename("/etc/opkg.temp","/etc/opkg.conf") )
        {
            cloudc_debug("%s[%d]: Exit,rename success", __func__, __LINE__);
            return 0;
        }
        else
        {
            cloudc_debug("%s[%d]: Exit,rename failure", __func__, __LINE__);
            return 1;
        }
    }
}

int cloudc_download_plugin(char *download_url)
{
    CURL *curl;
    CURLcode res;
    int download_status = -1;
    struct FtpFile ftpfile={
        "/etc/download.temp", /* name to store the file as if succesful */
        NULL
    };

    cloudc_debug("%s[%d]: Enter, and url is %s", __func__, __LINE__, download_url);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        /*
         * You better replace the URL with one that works! Note that we use an
         * FTP:// URL with standard explicit FTPS. You can also do FTPS:// URLs if
         * you want to do the rarer kind of transfers: implicit.
         */
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        /* Define our callback to get called when there's data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        /* Switch on full protocol/debug output */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        /* always cleanup */
        curl_easy_cleanup(curl);

        if(CURLE_OK != res) {
            /* we failed */
            cloudc_debug("%s[%d]: curl told us %d\n", __func__, __LINE__, res);
            download_status = 1;
        }
    }

    if(ftpfile.stream)
        fclose(ftpfile.stream); /* close the local file */

    curl_global_cleanup();

    if(1 == download_status)
    {
        cloudc_debug("%s[%d]: Exit,download opkg conf failure", __func__, __LINE__);
        return 1;
    }
    else
    {
        if(0 == rename("/etc/download.temp","/etc/download.ipk") )
        {
            cloudc_debug("%s[%d]: Exit,rename success", __func__, __LINE__);
            return 0;
        }
        else
        {
            cloudc_debug("%s[%d]: Exit,rename failure", __func__, __LINE__);
            return 1;
        }
    }
}

int cloudc_install_plugin(char *ipk_name)
{
    int check_result = -1;
    char temp_buffer[128];

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    memset(temp_buffer, 0, 128);

    strcpy(temp_buffer, "opkg install /etc/download.ipk");
    system(temp_buffer);

    check_result = check_plugin_installed(ipk_name);

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return check_result;
}

int cloudc_remove_plugin(char *ipk_name)
{
    int check_result = -1;
    char temp_buffer[128];

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    memset(temp_buffer, 0, 128);

    strcpy(temp_buffer, "opkg remove ");
    strcat(temp_buffer, ipk_name);
    system(temp_buffer);

    check_result = check_plugin_installed(ipk_name);

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return check_result;
}

int cloudc_install_ipk(char *ipk_name)
{
    int package_len = strlen(ipk_name);
    int check_result = -1;

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    if ( NULL != ipk_name)
    {
        char * temp_buffer = (char *)malloc(package_len + LEN_OF_CMD_HEAD);
        if (NULL == temp_buffer)
        {
            cloudc_error("%s[%d]: malloc failed", __func__, __LINE__);
            return IPK_UNINSTALLED;
        }
        strcpy(temp_buffer, "opkg-customer install ");
        strcat(temp_buffer, ipk_name);
        system("opkg-customer update");
        system(temp_buffer);

        free(temp_buffer);

        check_result = check_ipk_installed(ipk_name);
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    }
    else
    {
        check_result = IPK_UNINSTALLED;
    }

    if (check_result == IPK_INSTALLED)
        return 0;
    else
        return 1;
}

int cloudc_upgrade_ipk(char *ipk_name)
{
    int check_result_1 = -1;
    int check_result_2 = -1;

    if (NULL != ipk_name)
    {
        cloudc_uninstall_ipk(ipk_name);
        check_result_1 = check_ipk_installed(ipk_name);
        cloudc_install_ipk(ipk_name);
        check_result_2 = check_ipk_installed(ipk_name);
    }
    else
    {
        return -1;
    }

    if (check_result_1 == IPK_UNINSTALLED && check_result_2 == IPK_INSTALLED)
        return 0;
    else
        return -1;
}

int cloudc_uninstall_ipk(char *ipk_name)
{
    /* To do: need to check if the ipk is installed firstly
     * if not installed, just return to tell server this is a wrong commond
     * if installed, just excute 
     * */
    int package_len = strlen(ipk_name);
    int check_result = -1;

    if (NULL != ipk_name)
    {
        char * temp_buffer = (char *)malloc(package_len + LEN_OF_CMD_HEAD);
        if (NULL == temp_buffer)
        {
            cloudc_error("%s[%d]: malloc failed", __func__, __LINE__);
            return IPK_UNINSTALLED;
        }
        strcpy(temp_buffer, "opkg-customer remove ");
        strcat(temp_buffer, ipk_name);
        system(temp_buffer);
        free(temp_buffer);

        check_result = check_ipk_installed(ipk_name);
        cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    }
    else
    {
        check_result == IPK_UNINSTALLED;
    }

    if (check_result == IPK_UNINSTALLED)
        return 0;
    else
        return 1;
}

int cloudc_get_running_ipk_list(char *op_type, int serial_num)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    char temp_running_ipk_list[][MAX_IPK_NAME_LEN] = {"portal","loudc","hello"};
    int query_ipk_num = 3;

    //cloudc_send_rsp_query_buf(op_type, serial_num, temp_running_ipk_list, query_ipk_num);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}

int cloudc_get_install_ipk_list(char *op_type, int serial_num)
{
    FILE *fstream;
    char temp_string[50];

    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    system("opkg-customer list-customer-installed > /tmp/customer_ipk_list");

    fstream = fopen("/tmp/customer_ipk_list", "r");
    if (fstream==NULL)
    {
        cloudc_debug("%s[%d]: fail to open customer_ipk_list", __func__, __LINE__);
        return -1;
    }

    IPK_QUERY_INFO_NODE *pHead = malloc(sizeof(IPK_QUERY_INFO_NODE));
    pHead->pNext = NULL;
    IPK_QUERY_INFO_NODE *pTail = pHead;
    IPK_QUERY_INFO_NODE *pTemp;

    while(fgets(temp_string, 50, fstream) != NULL)
    {
        cloudc_debug("%s[%d]: temp_string is %s", __func__, __LINE__, temp_string);
        if(strlen(temp_string) > MAX_IPK_NAME_LEN)
        {
            cloudc_debug("%s[%d]: ipk_name is too long", __func__, __LINE__);
            return 1;
        }

        pTemp = malloc(sizeof(IPK_QUERY_INFO_NODE));

        temp_string[strlen(temp_string) - 1] = '\0'; /* replace the last '\n' with '0'*/
        cloudc_debug("%s[%d]: temp_string = %s", __func__, __LINE__, temp_string);
        strncpy(pTemp->ipk_query_name, temp_string, MAX_IPK_NAME_LEN - 1);

        pTemp->pNext = NULL;
        pTail->pNext = pTemp;
        pTail = pTemp;
    }
    fclose(fstream);

    cloudc_debug("%s[%d]: prepare to send rsp query buf", __func__, __LINE__);
    cloudc_send_rsp_query_buf(op_type, serial_num, pHead);
    cloudc_debug("%s[%d]: already finish to send rsp query buf", __func__, __LINE__);

    /*free all node*/
    pTemp = pHead->pNext;
    cloudc_debug("%s[%d]: pHead=%p, pTemp=%p", __func__, __LINE__, pHead, pTemp);
    while(pHead != NULL)
    {
        free(pHead);
        pHead = pTemp;
        if(pTemp->pNext != NULL)
            pTemp = pTemp->pNext;
    }

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}

int cloudc_start_ipk(char *ipk_name)
{
    char cmd[128] = {0};
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);

    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_stop_ipk(char *ipk_name)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_enable_ipk(char *ipk_name)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int cloudc_disable_ipk(char *ipk_name)
{
    cloudc_debug("%s[%d]: Enter", __func__, __LINE__);
    cloudc_debug("%s[%d]: Exit", __func__, __LINE__);
    return 0;
}

int check_ipk_installed(char *ipk_name)
{
    FILE *fstream;
    char temp_string[50];

    system("opkg-customer list-customer-installed > /tmp/customer_ipk_list");

    fstream = fopen("/tmp/customer_ipk_list", "r");
    if (fstream==NULL)
    {
        cloudc_debug("%s[%d]: fail to open customer_ipk_list", __func__, __LINE__);
        return -1;
    }

    cloudc_debug("%s[%d]: ipk_name is %s", __func__, __LINE__, ipk_name);
    while(fgets(temp_string, 50, fstream) != NULL)
    {
        cloudc_debug("%s[%d]: temp_string is %s", __func__, __LINE__, temp_string);
        if(temp_string == strstr(temp_string, ipk_name))
        {
            cloudc_debug("%s[%d]: ipk installed", __func__, __LINE__);
            fclose(fstream);
            return 0;
        }
    }
    fclose(fstream);
    cloudc_debug("%s[%d]: ipk not installed", __func__, __LINE__);
    return 1;
}

int check_plugin_installed(char *ipk_name)
{
    FILE *fstream;
    char temp_string[50];
    char list_plugin[64];

    memset(list_plugin, 0, 64);

    strcpy(list_plugin, "opkg list-installed | grep ");
    strcat(list_plugin, ipk_name);
    strcat(list_plugin, " > /tmp/customer_ipk_list");

    system(list_plugin);

    fstream = fopen("/tmp/customer_ipk_list", "r");
    if (fstream==NULL)
    {
        cloudc_debug("%s[%d]: fail to open customer_ipk_list", __func__, __LINE__);
        return -1;
    }

    cloudc_debug("ipk_name is %s", ipk_name);
    while(fgets(temp_string, 50, fstream) != NULL)
    {
        cloudc_debug("temp_string is %s",temp_string);
        if(NULL != strstr(temp_string, ipk_name))
        {
            cloudc_debug("ipk installed");
            fclose(fstream);
            return 0;
        }
    }
    fclose(fstream);
    cloudc_debug("ipk not installed");
    return 1;
}

int cloudc_opkg_conf_update()
{
    FILE *fstream;
    int temp;

    system("opkg-customer update 2>/tmp/opkg_update_result");
    fstream = fopen("/tmp/opkg_update_result", "r");
    if (fstream != NULL)
    {
        if((temp=fgetc(fstream)) != EOF)
        {
            cloudc_debug("%s[%d]: opkg update error", __func__, __LINE__);
            fclose(fstream);
            return 1;
        }
    }

    cloudc_debug("%s[%d]: opkg update success", __func__, __LINE__);
    fclose(fstream);
    return 0;
}
