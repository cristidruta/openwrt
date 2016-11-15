#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "cJSON.h"
#include "pCtlIntf.h"


extern int PCtlMsg_send(const PCtlMsgHeader *buf);
#if 0
#define BUFF_SIZE 4096

#define MSG_CONNECT_CMD     0x10
#define MSG_SUBSCRIBE_REQ   0x80
#define MSG_PING_REQ        0xc0

static int pingCount=0;
char g_connAckRepl[4]={0x20, 0x02, 0x00, 0x00};
char g_subAckRepl[5]={0x90, 0x03, 0x00, 0x01, 0x00};
char g_pingRepl[2]={0xd0, 0x00};

static int getColor(char *color)
{
    char line[256]={0};
    FILE* fp=NULL;
    char c, *dst=color+1;
    int i=0;

    /* init with dirty */
    strcpy(line, "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");

    if ((fp=fopen("/var/led_color", "r")) ==NULL)
    {
        printf("open /var/led_color failed! %s\r\n", strerror(errno));
        return -1;
    }

    if (!fgets(line, sizeof(line), fp))
    {
        printf("read /var/led_color failed! %s\r\n", strerror(errno));
        close(fp);
        return -1;
    }
    close(fp);

    *color = '"';

    for(i=0; i<24; i++)
    {
        c=line[i];
        if ((c>='0' && c <= '9') ||
            (c>='a' && c <= 'f'))
        {
            *(dst+i)=c;
        }
        else
        {
            printf("content of /var/led_color is wrong!\r\n");
            return -1;
        }
    }

    *(dst+i)='"';
    *(dst+i+1)='\0';

    printf("color=%s\r\n", color);
    return 0;
}



int led_color()
{
    int ret=0;
    char color[128]={0};

    if (getColor(color)!= 0)
    {
        printf("getColor failed!\r\n");
        return 0;
    }
    ret=sendToLed("\"2\":\"2\",\"4\"", color);
    return ret;
}

int main_service(int sockfd)
{
    unsigned char buff[BUFF_SIZE];
    size_t len;
    int ret=0;
    int i=0;
    unsigned char msgType=0;

    len=read(sockfd, (char*)buff, BUFF_SIZE);

    if (len == 0)
    {
        return -1;
    }

    if (len < 0)
    {
        if (errno == ECONNRESET)
        {
            return -1;
        }

        printf("read error: %s!\r\n", strerror(errno));
        return 0;
    }

    //业务逻辑从这里开始
    printf("recv %d data:\r\n", len);
    for(i=0; i<len; i++) {
        printf("%02x ", buff[i]);
        if((i+1)%20 == 0) {
            printf("\r\n");
        }
    }
    printf("\r\n======================\r\n");
        
    msgType=((unsigned char)buff[0]) & 0xf0;
    switch(msgType)
    {
        case MSG_CONNECT_CMD:
            /* Connect Command */
            ret=write(sockfd, g_connAckRepl, 4);
            printf("[Connect Command], ret=%d\r\n", ret);
            break;

        case MSG_SUBSCRIBE_REQ:
            /* Subscribe Request */
            ret=write(sockfd, g_subAckRepl, 5);
            printf("[Subscribe Request], ret=%d\r\n", ret);
            break;

        case MSG_PING_REQ:
            /* Ping Request */
            ret=write(sockfd, g_pingRepl, 2);
            pingCount++;
            printf("[Ping Request], ret=%d pingCount=%d\r\n", ret, pingCount);
            //ret=led_onOff(sockfd, pingCount%2);
            break;
    
        default:
            printf("[Unknown MSG], type=0x%2x\r\n", msgType);
            break;
    }

    if(ret<0)
    {
        printf("error: %s!\r\n", strerror(errno));
        return -1;
    }

    return 0;
}
#endif


static int sendToLed(char *preStr, char *valStr)
{
    char buf[512]={0};
    PCtlMsgHeader *msg=(PCtlMsgHeader *)buf;
    char *ptr=(char*)(msg+1);
    int len=0, ret=0;
    char *cmdFmt="smart/gw/002000345ccf7f1b7e15{\"protocol\":5,\"type\":null,\"gwId\":null,\"data\":{\"gwId\":\"002000345ccf7f1b7e15\",\"devId\":\"002000345ccf7f1b7e15\",\"dps\":{%s:%s}},\"pv\":\"1.0\",\"t\":%lu,\"sign\":null}";
    /* mb mode */
    //char *cmdFmt="smart/mb/002000345ccf7f1b7e15{\"protocol\":4,\"data\":{\"gwId\":\"002000345ccf7f1b7e15\",\"devId\":\"002000345ccf7f1b7e15\",\"dps\":{\"1\":%s}},\"t\":%lu}";

    msg->type = PCTL_MSG_DEV_SET;

    ptr[0] = 0x30;
    //buf[1] = 0x30; /* length */
    ptr[2] = 0x01;
    ptr[3] = 0x00;
    ptr[4] = 0x1d;

    len = snprintf(ptr+5, 255, cmdFmt, preStr, valStr, (unsigned long)time(NULL));
    ptr[1] = len+2;

    msg->dataLength = len+5;

    adapt_debug("[Set Led], len=%d cmd={%s:%s}\r\n", len+5, preStr, valStr);

    if ((ret=PCtlMsg_send(msg)) < 0)
    {
        adapt_error("PCtlMsg_send() failed! ret=%d\r\n", ret);
    }

    return ret;
}

int handle_settings(char *newCfg, char *devData)
{
    cJSON *newJson=NULL, *devJson=NULL;
    cJSON *jsonItem=NULL;
    int ret=-1;

    if (!newCfg || !devData)
    {
        adapt_error("invalid param!");
        return -1;
    }

    if ((newJson=cJSON_Parse(newCfg)) == NULL)
    {
        adapt_error("parse settings %s failed!", newCfg);
        return -1;
    }

    if ((devJson=cJSON_Parse(devData)) == NULL)
    {
        adapt_error("parse devData %s failed!", devData);
        goto exit1;
    }

    if ((jsonItem=cJSON_GetObjectItem(newJson, "power")) != NULL)
    {
        if (jsonItem->type == cJSON_String)
        {
            if (jsonItem->valuestring)
            {
                if (strcmp(jsonItem->valuestring, "0") == 0)
                {
                    if ((ret=sendToLed("\"1\"", "false"))<0)
                    {
                        adapt_error("set led off failed!");
                        goto exit;
                    }
                }
                else if (strcmp(jsonItem->valuestring, "1") == 0)
                {
                    if ((ret=sendToLed("\"1\"", "true"))<0)
                    {
                        adapt_error("set led on failed!");
                        goto exit;
                    }
                }
                else
                {
                    adapt_error("value of power %s is invalid!", jsonItem->valuestring);
                }
            }
        }
    }

exit:
    cJSON_Delete(devJson);
exit1:
    cJSON_Delete(newJson);

    return ret;
}
