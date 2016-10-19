#ifndef  _PROTO_H_
#define  _PROTO_H_

#include "parser.h"
#include "handle_sys_feature.h"

#ifdef __cplusplus
extern "C"
{
#endif

void iota_genHostStr(char *server);
int cloudc_build_register_js_buf(char *js_buf); 
int cloudc_build_online_js_buf(char *js_buf, char *devData); 
int cloudc_build_recv_rsp_js_buf(char *type, int serial, int rsp_status, char *js_buf);
int cloudc_build_alljoyn_recv_rsp_js_buf(char *type, int serial, char *user_id, char *device_id, int rsp_status, char *js_buf);
int cloudc_build_rsp_ipk_js_buf(char *type, int serial, struct ipk_info *ipk_list_head, int *status, int real_ipk_num, char *js_buf);
int cloudc_build_rsp_query_js_buf(char *type, int serial, struct ipk_query_info_node *query_list_head, char *js_buf);
int cloudc_build_rsp_opkg_js_buf(char *type, int serial, int update_status, int replace_status, char *js_buf);
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



#ifdef __cplusplus
}
#endif



#endif
