#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mxml.h>
#include "cJSON.h"
#include "share.h"
#include "log.h"

int parseDevOnlineBuf(char *jsonBuf, char *manufacture, char *moduleNumber, char *deviceType, char *devSn, char *tansportType);
int updateConfigInfo(char *updateBuf, char *deviceType, char *manufacture, char *moduleNumber, char *devSn, char *configName, char *configValue);
int convertFromJsonToStr(char *json, char *str);
int parseDevPower(char *jsonBuf, char *power);
int parseManufactureSN(char *jsonBuf,char *devSn);

int parseManufactureSN(char *jsonBuf,char *devSn)
{
    cJSON *json, *jsonDeviceSN;
    
    json = cJSON_Parse(jsonBuf);

    if (!json)
    {
        adapt_error("not json format\n ");
        adapt_error("Error before: [%s]\n",cJSON_GetErrorPtr());
        return -1;
    }
    else
    {
        jsonDeviceSN = cJSON_GetObjectItem(json, "manufactureSN");
        if (NULL == jsonDeviceSN)
        {
            adapt_error("failed to get manufactureSN from json\n");
            return -1;
        }
        if (jsonDeviceSN->type == cJSON_String )
        {
            strncpy(devSn, jsonDeviceSN->valuestring, MAX_MANUFACTURESN_LEN - 1);
            adapt_debug("devSn = %s\n", devSn);
        }
        else
        {
            //printf("manufacture value is not string\n");
            adapt_debug("manufactureSN value is not string\n");
        }

    }
    cJSON_Delete(json);  
}
int parseDevPower(char *jsonBuf, char *power)
{
    cJSON *json, *jsonPower;
    
    json = cJSON_Parse(jsonBuf);

    if (!json)
    {
        adapt_error("not json format\n ");
        adapt_error("Error before: [%s]\n",cJSON_GetErrorPtr());
        return -1;
    }
    else
    {
        jsonPower = cJSON_GetObjectItem(json, "power");
        if (NULL == jsonPower)
        {
            adapt_error("failed to get power from json\n");
            return -1;
        }
        if (jsonPower->type == cJSON_String )
        {
            strncpy(power, jsonPower->valuestring, MAX_POWER_LEN - 1);
            adapt_debug("power = %s\n", power);
        }
        else
        {
            //printf("manufacture value is not string\n");
            adapt_debug("Power value is not string\n");
        }

    }
    cJSON_Delete(json);  
}


int parseDevOnlineBuf(char *jsonBuf, char *manufacture, char *moduleNumber, char *deviceType, char *devSn, char *transportType)
{
    cJSON *json, *jsonManufacture, *jsonModuleNumber, *jsonDeviceType, *jsonDeviceSn, *jsonTransport;
    int parseStatus = 0;

    /* parse jsonBuf */
    json = cJSON_Parse(jsonBuf);  

    if (!json)  
    {   
        //printf("not json format\n ");
        adapt_error("not json format\n ");
        //printf("Error before: [%s]\n", cJSON_GetErrorPtr());  
        adapt_error("Error before: [%s]\n", cJSON_GetErrorPtr());  
        return -1;
    }   

    else  
    {   
        /* parse item "manufacture" */ 
        jsonManufacture = cJSON_GetObjectItem(json, "manufacture");

        if (NULL == jsonManufacture)
        {
            //printf("failed to get manufacture from json\n");
            adapt_error("failed to get manufacture from json\n");
            return -1;
        }
        if (jsonManufacture->type == cJSON_String )   
        {  
            parseStatus ++;
            strncpy(manufacture, jsonManufacture->valuestring, MAX_MANUFACTURE_LEN - 1);
            //printf("manufacture = %s\n", manufacture);  
            adapt_debug("manufacture = %s\n", manufacture);  
        }  
        else
        {
            //printf("manufacture value is not string\n");
            adapt_debug("manufacture value is not string\n");
        }

        /* parse item "moduleNumber" */ 
        jsonModuleNumber = cJSON_GetObjectItem(json, "manufactureDataModelId");

        if (NULL == jsonModuleNumber)
        {
            //printf("failed to get moduleNumber from json\n");
            adapt_error("failed to get moduleNumber from json\n");
            return -1;
        }
        if (jsonModuleNumber->type == cJSON_String )   
        {  
            parseStatus ++;
            strncpy(moduleNumber, jsonModuleNumber->valuestring, MAX_MODULENUMBER_LEN - 1);
            //printf("moduleNumber = %s\n", moduleNumber);  
            adapt_debug("moduleNumber = %s\n", moduleNumber);  
        }  
        else
        {
            //printf("moduleNumber value is not string\n");
            adapt_debug("moduleNumber value is not string\n");
        }

        /* parse item "deviceType" */ 
        jsonDeviceType = cJSON_GetObjectItem(json, "deviceType");

        if (NULL == jsonDeviceType)
        {
            //printf("failed to get deviceType from json\n");
            adapt_error("failed to get deviceType from json\n");
            return -1;
        }
        if (jsonDeviceType->type == cJSON_String )   
        {  
            parseStatus ++;
            strncpy(deviceType, jsonDeviceType->valuestring, MAX_DEVICETYPE_LEN - 1);
            //printf("deviceType = %s\n", deviceType);  
            adapt_debug("deviceType = %s\n", deviceType);  
        }  
        else
        {
            //printf("deviceType value is not string\n");
            adapt_debug("deviceType value is not string\n");
        }
        /* parse item "serialNumber" */ 
        jsonDeviceSn = cJSON_GetObjectItem(json, "manufactureSN");

        if (NULL == jsonDeviceSn)
        {
            //printf("failed to get devSn from json\n");
            adapt_error("failed to get devSn from json\n");
            return -1;
        }
        if (jsonDeviceSn->type == cJSON_String )   
        {  
            parseStatus ++;
            strncpy(devSn, jsonDeviceSn->valuestring, MAX_DEVICESN_LEN - 1);
            //printf("devSn = %s\n", devSn);  
            adapt_debug("devSn = %s\n", devSn);  
        }  
        else
        {
            //printf("devSn value is not string\n");
            adapt_debug("devSn value is not string\n");
        }

        /* parse item "transportType" */ 
        jsonTransport = cJSON_GetObjectItem(json, "transportType");

        if (NULL == jsonTransport)
        {
            //printf("failed to get transportType from json\n");
            adapt_error("failed to get transportType from json\n");
            return -1;
        }

        if (jsonTransport->type == cJSON_String )   
        {  
            parseStatus ++;
            strncpy(transportType, jsonTransport->valuestring, MAX_TRANSPORT_LEN - 1);
            //printf("transportType = %s\n", transportType);  
            adapt_debug("transportType = %s\n", transportType);  
        }  
        else
        {
            //printf("transport value is not string\n");
            adapt_debug("transport value is not string\n");
        }

        
        cJSON_Delete(json);  
        /* free */
    }

    if(parseStatus == 5)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int convertFromJsonToStr(char *json, char *str)
{
    cJSON *jsonNode;
    jsonNode = cJSON_Parse(json);
    if(jsonNode != NULL)
    {
        char *s = cJSON_PrintUnformatted(jsonNode);
        if(s)
        {
            memset(str, 0, MAX_DEVDATA_LEN);
            strncpy(str, s, MAX_DEVDATA_LEN - 1);    
            free(s);
        }
        cJSON_Delete(jsonNode);
    }
    return 0;

}


