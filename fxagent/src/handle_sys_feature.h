#ifndef _HANDLE_SYS_FEATURE_ 
#define _HANDLE_SYS_FEATURE_

#ifdef __cplusplus
extern "C"
{
#endif


#define LEN_OF_CMD_HEAD 20
#define IPK_INSTALLED 0
#define IPK_UNINSTALLED 1


struct FtpFile 
{
    const char *filename;
    FILE *stream;
};

typedef struct ipk_query_info_node 
{
    char ipk_query_name[MAX_IPK_NAME_LEN];
    struct ipk_query_info_node *pNext;
}IPK_QUERY_INFO_NODE;

int get_ap_info(void);
void cloudc_rpc_method_handle(struct http_value recv_data);
int cloudc_manage_ipk(char *op_type, int serial_num, struct ipk_info *ipk_list_head, int real_ipk_num);
int cloudc_manage_query(char *op_type, int serial_num);
int cloudc_manage_opkg(char *op_type, int serial_num, int update_flag, char *url);
int cloudc_manage_service(char *op_type, int serial_num, struct ipk_info *ipk_list_head, int real_ipk_num);
int cloudc_manage_alljoyn_set_operation(char *op_type, int serial_num, char *user_id, char *device_id, char *device_type, char *devData);
int cloudc_manage_alljoyn_update_devid(char *op_type, char *device_id, char *device_sn);
int cloudc_manage_alljoyn_set_devData(char *deviceId);

int cloudc_download_file(char *download_url);
int cloudc_opkg_conf_update();

int cloudc_install_ipk(char *ipk_name);
int cloudc_upgrade_ipk(char *ipk_name);
int cloudc_uninstall_ipk(char *ipk_name);
int check_ipk_installed(char *ipk_name);

int cloudc_get_running_ipk_list(char *op_type, int serial_num);
int cloudc_get_install_ipk_list(char *op_type, int serial_num);

int cloudc_start_ipk(char *ipk_name);
int cloudc_stop_ipk(char *ipk_name);
int cloudc_enable_ipk(char *ipk_name);
int cloudc_disable_ipk(char *ipk_name);


#ifdef __cplusplus
}
#endif


#endif
