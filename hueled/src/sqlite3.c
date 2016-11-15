#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include "share.h"
#include "log.h"

#define SQLCMD_BASE_LEN 128
#define MAX_SQLCMD_LEN 4096
#define DEVICE_DATABASE_PATH "/var/device.db"

//#define MAX_MANUFACTURESN_LEN 32
//#define MAX_MANUFACTURE_LEN 32

sqlite3 *db = NULL;
int needCreateFlag = TRUE;


int find_device_table_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;

    adapt_debug("deviceinfo table already existed");
    needCreateFlag = FALSE;

    for(i = 0; i < argc; i++){
        adapt_debug("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    return 0;
}

int sqlModuleInit()
{
    int ret=0;
    char *errMsg=NULL;
    char *sqlQueryTable="select * from sqlite_master where type='table' and name='deviceinfo';";
    char *sqlSynOff="PRAGMA synchronous = OFF;";

    /*create table and save the info */
    ret = sqlite3_open(DEVICE_DATABASE_PATH, &db);
    if (ret != SQLITE_OK) {
        adapt_error("open sqlite3 %s, error[%s]", DEVICE_DATABASE_PATH, sqlite3_errmsg(db));
        return -1;
    }

    adapt_debug("sqlite open succ");

    /* create table */ 
    ret = sqlite3_exec(db, sqlQueryTable, find_device_table_callback, 0, &errMsg);
    if (ret != SQLITE_OK) { 
        adapt_error("query table deviceinfo Error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }

    adapt_debug("needCreateFlag=%d ", needCreateFlag);

    if (needCreateFlag == TRUE) {
        adapt_debug("sqlite create table deviceinfo ");
        char *sqlCreateTable = "CREATE TABLE deviceinfo(interface TEXT PRIMARY KEY ASC, \
                                objpath TEXT, platformid TEXT, manufacture TEXT, modulenumber TEXT, \
                                devicetype TEXT, deviceid TEXT, devicesn TEXT, STATUS TEXT, devData TEXT);";

        ret = sqlite3_exec(db, sqlCreateTable, NULL, 0, &errMsg);
        if (ret != SQLITE_OK) { 
            adapt_error("create table deviceinfo Error: %s\n",  sqlite3_errmsg(db));
            sqlite3_free(errMsg);
            return -1;
        }
        adapt_debug("sqlite create table deviceinfo succeed ");
    }

    adapt_debug("set syn mode to off");
    ret = sqlite3_exec(db, sqlSynOff,  NULL, 0, &errMsg);
    if (ret != SQLITE_OK) {
        adapt_error("Set Syn mode Failed: %s\n",  sqlite3_errmsg(db));
        sqlite3_free(errMsg);
        return -1;
    } else {
        adapt_debug("set syn mode succ");
    }

    return 0;
}

int sqlUpdateDevData(char *manufacture,
                     char *sn, 
                     char *intfName,
                     char *objPath,
                     char *moduleNumber, 
                     char *devType, 
                     char *devData)
{
    char sqlCmd[MAX_SQLCMD_LEN]={0};
    char *errMsg=NULL;
    int ret=-1;

    snprintf(sqlCmd, sizeof(sqlCmd)-1, 
            "INSERT INTO deviceinfo VALUES(\"%s\", \"%s\", \"\", \"%s\", \"%s\", \"%s\", \"\", \"%s\", \"on\", \'%s\');",
            intfName, objPath, manufacture, moduleNumber, devType, sn, devData);

    adapt_debug("sqlCmd=%s\n", sqlCmd);

    ret=sqlite3_exec(db, sqlCmd, NULL/*sqliteCallback*/, 0, &errMsg);
    if (ret != SQLITE_OK) {
        adapt_error("insert value Error: %s\n", sqlite3_errmsg(db));
        sqlite3_free(errMsg);
        return -1;
    }
    
    adapt_debug("insert succeed");
    return 0;
}

int getDevStsByDevSn(char *manufacture, char *deviceSn)
{
    int ret=-1;
    char **dbresult;
    char *errMsg=NULL;
    char sqlCmd[SQLCMD_BASE_LEN]={0};
    char devSts[MAX_ONLINESTATUS_LEN]={0};
    int nrow=0,ncolumn=0;

    adapt_debug("Enter.");

    snprintf(sqlCmd, sizeof(sqlCmd), "select STATUS from deviceinfo WHERE manufacture = '%s' AND deviceSn = '%s'", manufacture, deviceSn);
    adapt_debug("sql cmd=%s\n", sqlCmd);
    ret = sqlite3_get_table(db, sqlCmd, &dbresult, &nrow, &ncolumn, &errMsg);
    if (ret != SQLITE_OK) {
        adapt_error("sqlite3_get_table error[%s]", errMsg);
        sqlite3_free(errMsg);
        goto out;
    }

    adapt_debug("query result: nrow = %d, ncolumn = %d\n", nrow, ncolumn);

    if (nrow != 0) {
        strncpy(devSts, dbresult[ncolumn], MAX_ONLINESTATUS_LEN);
        adapt_debug("devSts: %s", devSts);
    }

    if (nrow > 1) {
        adapt_error("db error, have multiple rows for %s:%s", manufacture, deviceSn);
    }

    if (strcmp(devSts, "on")==0) {
        ret=1;
    } else if (strcmp(devSts, "off")==0) {
        ret=0;
    }

out:
    sqlite3_free_table(dbresult);

    adapt_debug("Exit.");
    return ret;
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

void getDevInfoByDevId(char *deviceId, char *manufactureSN, char *manufacture)
{
    /* connect to database */
    //sqlite3 *db = 0;
    char * errmsg = 0;
    int ret = 0;

    adapt_debug("%s[%d]: Enter", __func__, __LINE__);
    //int ret = sqlite3_open(DEVICE_DATABASE_PATH, &db);

    //if(ret == SQLITE_OK)
    {
        //adapt_debug("%s[%d]: You have opened a sqlite3 database named device.db successfully!", __func__, __LINE__);
        /* query the data you want */
        char **dbresult;
        char *errmsg = 0;
        char sqlCmd[SQLCMD_BASE_LEN] = {0};
        int i = 0;
        int j = 0;
        int nrow = 0;
        int ncolumn = 0;
        int index = 0; 

        snprintf(sqlCmd, sizeof(sqlCmd), "select devicesn, manufacture from deviceinfo WHERE deviceid = '%s'", deviceId );
        adapt_debug("%s[%d]: sql cmd = %s", __func__, __LINE__, sqlCmd);
        ret = sqlite3_get_table(db, sqlCmd, &dbresult, &nrow, &ncolumn, &errmsg);
        adapt_debug("%s[%d]: query result: nrow = %d, ncolumn = %d", __func__, __LINE__, nrow, ncolumn);
        if(ret == SQLITE_OK)
        {
            adapt_debug("%s[%d]: query %i records.", __func__, __LINE__, nrow);
            index = ncolumn;

            for(i = 0; i < nrow; i++)
            {
                adapt_debug("%s[%d]: content in row [%d], index = %d:", __func__, __LINE__, i, index);
                for(j = 0; j < ncolumn; j++)
                { 
                    adapt_debug("%s[%d]: j = %d %s\n", __func__, __LINE__, j, dbresult[index]);
                    switch(j)
                    {
                        case 0:
                    //        memset(manufactureSN, 0, MAX_MANUFACTURESN_LEN);
                            strncpy(manufactureSN, dbresult[index], MAX_MANUFACTURESN_LEN - 1);
                            manufactureSN[MAX_MANUFACTURESN_LEN-1] = '\0';
                            adapt_debug("%s[%d]: manufactureSN = %s", __func__, __LINE__, manufactureSN);
                            break;
                        case 1:
                      //      memset(manufacture, 0, MAX_MANUFACTURE_LEN);
                            strncpy(manufacture, dbresult[index], MAX_MANUFACTURE_LEN - 1);
                            manufacture[MAX_MANUFACTURE_LEN-1] = '\0';
                            adapt_debug("%s[%d]: manufacture = %s", __func__, __LINE__, manufacture);
                        default:
                            break;
                    }
                    index++;
                }
            }
        }
        else
        {
            adapt_debug("%s[%d]: query failed", __func__, __LINE__);
            fprintf(stderr,"Error: %s\n", sqlite3_errmsg(db));
            sqlite3_free(errmsg);
        }
        sqlite3_free_table(dbresult);
    }
#if 0
    else
    {
        cloudc_error("%s[%d]: %s", __func__, __LINE__, DEVICE_DATABASE_PATH);
        fprintf(stderr,"failed to open this db file: %s\n", sqlite3_errmsg(db));
        sqlite3_free(errmsg);
    }

    /* close */
    //sqlite3_close(db);
    //db = 0;
#endif
    adapt_debug("%s[%d]: Exit", __func__, __LINE__);
    return;

}
