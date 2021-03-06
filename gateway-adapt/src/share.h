#ifndef _SHARE_H_
#define _SHARE_H_

#ifdef __cplusplus
extern "C"
{
#endif


//#include <sqlite3.h>

#define MAX_INTERFACE_LEN 128
#define MAX_OBJECTPATH_LEN 128
#define MAX_DEVICETYPE_LEN 32
#define MAX_DEVICEID_LEN 32
#define MAX_DEVICESN_LEN 32
#define MAX_ONLINESTATUS_LEN 4
#define MAX_MANUFACTURE_LEN 16
#define MAX_MODULENUMBER_LEN 32
#define MAX_CONFIGNAME_LEN 32
#define MAX_CONFIGVALUE_LEN 32
#define MAX_TRANSPORT_LEN 32

#define MAX_DEVDATA_LEN 512
#define MAX_SENDBUF_LEN 1024 

#define DEVICE_DATABASE_PATH "/var/device.db"
#define DATA_CONVERT_FILE_PATH "/etc/config/DataConvert.xml"

#define FALSE   0
#define TRUE    1
//extern sqlite3 *mdb;
//extern sqlite3 *db;

#ifdef __cplusplus
}
#endif


#endif /* _SHARE_H_ */
