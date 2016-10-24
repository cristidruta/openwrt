#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "share.h"
#include "log.h"

#define SQLCMD_BASE_LEN 128


sqlite3 *db = NULL;
int needCreateFlag = TRUE;


int find_device_table_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    //int ret = 0;
    //char *errMsg = NULL;

    adapt_debug("deviceinfo table already existed");
    needCreateFlag = FALSE;
#if 0
    if(argc != 0){
       //std::cout << "deviceinfo table already existed" << std::endl;
       adapt_debug("deviceinfo table already existed");
    } else {
        //std::cout << "sqlite create table deviceinfo " << std::endl;
        adapt_debug("sqlite create table deviceinfo ");
        const char *sqlCreateTable = "CREATE TABLE deviceinfo(interface TEXT PRIMARY KEY ASC, \
                                      objpath TEXT, platformid TEXT, manufacture TEXT, modulenumber TEXT, \
                                      devicetype TEXT, deviceid TEXT, devicesn TEXT, STATUS TEXT, devData TEXT)";

        ret = sqlite3_exec(db, sqlCreateTable, NULL, 0, &errMsg);
        if (ret != SQLITE_OK) { 
            //fprintf(stderr,"create table deviceinfo Error: %s\n", sqlite3_errmsg(db));
            adapt_error("create table deviceinfo Error: %s\n",  sqlite3_errmsg(db));
            sqlite3_free(errMsg);
            //return 1;
        }
        //std::cout << "sqlite create table deviceinfo succeed " << std::endl;
        adapt_debug("sqlite create table deviceinfo succeed ");
    }
#endif

    for(i = 0; i < argc; i++){
        adapt_debug("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    return 0;
}


/* manufacture, devSn ->>>> onlineStatus */
int getOnlineStatusByDevSn(char *manufacture, char *deviceSn, char *onlineStatus, char *deviceType)
{
    int ret = 0;
    char **dbresult;
    char *errMsg = 0;
    char sqlCmd[SQLCMD_BASE_LEN] = {0};
    int nrow = 0;
    int ncolumn = 0;

    adapt_debug("enter getOnlineStatusByDevSn ");
    snprintf(sqlCmd, sizeof(sqlCmd), "select STATUS from deviceinfo WHERE manufacture = '%s' AND deviceSn = '%s'", manufacture, deviceSn);
    adapt_debug("sql cmd = %s\n", sqlCmd);
    ret = sqlite3_get_table(db, sqlCmd, &dbresult, &nrow, &ncolumn, &errMsg);
    if (ret != SQLITE_OK) {
        adapt_error("sqlite3_get_table error[%s]", errMsg);
        sqlite3_free(errMsg);
        goto err;
    }

    adapt_debug("query result: nrow = %d, ncolumn = %d\n", nrow, ncolumn);
    if (nrow > 1) {
        adapt_error("database error, have multiple result relate with manufacture[%s] deviceSn[%s]",
                manufacture, deviceSn);
        ret = -1;
        goto err;
    }

    if (nrow == 0) {
        strcpy(onlineStatus, "");
    } else {    
        strncpy(onlineStatus, dbresult[ncolumn], MAX_ONLINESTATUS_LEN);
        adapt_debug("onlineStatus[%s]", onlineStatus);
    }

err:
    sqlite3_free_table(dbresult);

    adapt_debug("Exit getOnlineStatusByDevSn");
    
    return 0;
}

int getDevDataByDevSn(char *deviceSn, char *devData)
{
    int ret = 0;
    char **dbresult;
    char *errMsg = 0;
    char sqlCmd[SQLCMD_BASE_LEN] = {0};
    int nrow = 0;
    int ncolumn = 0;

    adapt_debug("Enter getDevDataByDevSn\n");
    snprintf(sqlCmd, sizeof(sqlCmd), "select devData from deviceinfo WHERE devicesn = '%s'", deviceSn);
    adapt_debug("sql cmd = %s\n", sqlCmd);
    ret = sqlite3_get_table(db, sqlCmd, &dbresult, &nrow, &ncolumn, &errMsg);
    if (ret != SQLITE_OK) {
        adapt_error("sqlite3_get_table error[%s]", errMsg);
        sqlite3_free(errMsg);
        goto err;
    }
    adapt_debug("query result: nrow = %d, ncolumn = %d\n", nrow, ncolumn);

    if (nrow > 1) {
        adapt_error("database error, have multiple result relate with deviceSn[%s]",
                deviceSn);
        ret = -1;
        goto err;
    }

    strncpy(devData, dbresult[ncolumn], MAX_DEVDATA_LEN);
    adapt_debug("devData = %s\n", devData);

err:
    sqlite3_free_table(dbresult);

    adapt_debug("Exit getDevDataByDevSn");
    return ret;
}


int setDevDataByDevSn(char *deviceSn, char *devData)
{
    int ret = 0;
    char *errMsg = 0;
    char sqlCmd[SQLCMD_BASE_LEN+MAX_DEVDATA_LEN] = {0};

    adapt_debug("Enter setDevDataByDevSn\n");

    snprintf(sqlCmd, sizeof(sqlCmd), "UPDATE deviceinfo set devData = '%s' WHERE devicesn = '%s'", devData, deviceSn );
    adapt_debug("sql cmd = %s\n", sqlCmd);
    ret = sqlite3_exec(db, sqlCmd, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr,"update table deviceinfo Error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return ret;
    }
    else
    {
        adapt_debug("update devData value succeed\n");
    }

    adapt_debug("Exit setDevDataByDevSn\n");
    return ret;
}

