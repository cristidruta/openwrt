#include "share.h"

#define ADAPTER_PORT 6666
#define LOOPBACK_ADDR "127.0.0.1"

#define SRCADDR_LEN 2
#define NWKADDR_LEN 2
#define CAP_LEN 1

int sendJsonMsgToAdapt(char *recvBuf);

int sendJsonMsgToAdapt(char *recvBuf)
{
    smart_debug("%s[%d]: Enter", __func__, __LINE__);
    smart_debug("%s[%d]: recvBuf = %s", __func__, __LINE__, recvBuf);

    /* need to parse the recvBuf
     * then send proto it and send to adapter
     * */

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
    server_addr.sin_port = htons(ADAPTER_PORT);

    int client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if(client_socket_fd < 0)
    {   
        perror("Create Socket Failed:");
        return -1;
    }   

    if(sendto(client_socket_fd, recvBuf, SEND_MAX_BUF_LEN, 0, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0)
    {   
        perror("Send File Name Failed:");
        return -1;
    }   

    close(client_socket_fd);

    smart_debug("%s[%d]: Exit", __func__, __LINE__);

    return 0;
}



enum MT_STATE {
    SOP_STATE = 0,
    LEN_STATE,
    CMD_STATE1,
    CMD_STATE2,
    DATA_STATE,
    FCS_STATE
};


int MT_UartCalcFCS(char *msgPtr, int msgLen)
{
  char x;
  char xorResult;

  xorResult = 0;

  for ( x = 0; x < msgLen; x++, msgPtr++ )
    xorResult = xorResult ^ *msgPtr;

  return ( xorResult );

}


int parseZigbeeAnnceMsg(char *recvBuf, int bufLen, char *IEEEAddr, short *nwkAddrPtr)
{
    int cmd1;
    int cmd2;
    char data[RECV_MAX_BUF_LEN];
    int dataLen;
    short srcAddr;
    short nwkAddr;
    int i = 0;
    
    char *dataPtr = data;

    //printf("%s %d:len = %d \r\n", __func__, __LINE__, bufLen);
    smart_debug("%s %d:len = %d \r\n", __func__, __LINE__, bufLen);
    
    for (i = 0; i < bufLen; i++)
    {
        printf("0x%02x,", (unsigned char)recvBuf[i]);
    }
     
    printf("\r\n");

    if (parseZigbeeMTMsg(recvBuf, bufLen, &cmd1, &cmd2, data, &dataLen) < 0)
    {
        //printf("parseZigbeeMTMsg call error\r\n");
        smart_error("parseZigbeeMTMsg call error\r\n");
        return -1;
    }

    if ((cmd1 == 0x45) && (cmd2 == 0xC1) && (dataLen == SRCADDR_LEN + NWKADDR_LEN + IEEEADDR_LEN + CAP_LEN))
    {
        memcpy(&srcAddr, dataPtr, SRCADDR_LEN);
        dataPtr += SRCADDR_LEN;
        
        memcpy(nwkAddrPtr, dataPtr, NWKADDR_LEN);
        dataPtr += NWKADDR_LEN;
        
        memcpy(IEEEAddr, dataPtr, IEEEADDR_LEN);
        dataPtr += IEEEADDR_LEN;
        
    } else {

        smart_debug("not online message\r\n");
        //return -1;
    }

    return 0;


}


#define MT_UART_SOF 0xFE

int parseZigbeeMTMsg(char *recvBuf, int bufLen, int *cmd1, int *cmd2, char *data, int *dataLen)
{
    unsigned char ch;
    int len = 0;
    int state = SOP_STATE;
    char LEN_Token;
    char FSC_Token;
    
    while (len < bufLen)
    {
        ch = (unsigned char)recvBuf[len]; 
        switch (state)
        {
            case SOP_STATE:
                if (ch == MT_UART_SOF)
                    state = LEN_STATE;
                len++;
                break;

            case LEN_STATE:
                LEN_Token = ch;
                state = CMD_STATE1;
                *dataLen = LEN_Token;
                len++;
                break;

            case CMD_STATE1:
                *cmd1 = ch;
                state = CMD_STATE2;
                len++;
                break;

            case CMD_STATE2:
                *cmd2 = ch;
                len++;
                /* If there is no data, skip to FCS state */
                if (LEN_Token)
                {
                    state = DATA_STATE;
                }
                else
                {
                    state = FCS_STATE;
                }
                break;

            case DATA_STATE:
                memcpy(data, recvBuf + len, LEN_Token);
                len += LEN_Token; 
                state = FCS_STATE;

                break;

            case FCS_STATE:

                FSC_Token = ch;

                /* Make sure it's correct */
                /*if (MT_UartCalcFCS (recvBuf + 1, bufLen - 1) == FCS_Token)
                {
                    //printf("packet from zigbee device CRC error, ");
                    smart_error("packet from zigbee device CRC error, ");
                    return -1;
                }*/

                /* Reset the state, send or discard the buffers at this point */
                state = SOP_STATE;
                len++;

                break;

            default:
                break;
        }
    }

    return 0;
}


//#define IEEEADDR_HEX_LEN    17

int assembleJsonMsg(char *manufactureSN, char *jsonStrPtr)
{
    struct json_object *jsonObj;
    const char *jsonStr = NULL;
    //char IEEEAddrHex[IEEEADDR_HEX_LEN];
    int i = 0;
    
    /*for( i = 0; i < IEEEADDR_LEN; i++)
    {
       sprintf(IEEEAddrHex + i*2,"%02x", (unsigned char)manufactureSN[i]);
    }

    IEEEAddrHex[IEEEADDR_HEX_LEN] = '\0';*/
        
    jsonObj = json_object_new_object();

    json_object_object_add(jsonObj, "manufacture", json_object_new_string("feixun"));
    json_object_object_add(jsonObj, "moduleNumber", json_object_new_string("Flight"));
    json_object_object_add(jsonObj, "manufactureDataModelId", json_object_new_string("12"));
    json_object_object_add(jsonObj, "name", json_object_new_string("feixunFlight"));
    json_object_object_add(jsonObj, "manufactureSN", json_object_new_string(manufactureSN));
    json_object_object_add(jsonObj, "deviceType", json_object_new_string("led"));
    json_object_object_add(jsonObj, "line1", json_object_new_string("off"));
    json_object_object_add(jsonObj, "line2", json_object_new_string("off"));
    json_object_object_add(jsonObj, "line1Display", json_object_new_string("undefined"));
    json_object_object_add(jsonObj, "line2Display", json_object_new_string("undefined"));
    json_object_object_add(jsonObj, "transportType", json_object_new_string("zigbee"));
    json_object_object_add(jsonObj, "softwareversion", json_object_new_string("V1.0.0"));

    jsonStr = json_object_to_json_string(jsonObj);
   
    strcpy(jsonStrPtr, jsonStr);

    smart_debug("%s\r\n", jsonStr);

    return 0;

}

#define MSG_HEAD_LEN    3


void printMTMsg(unsigned char *MTMsg)
{
    int dataLen;
    int i = 0;

    /* print MT Message header */
    for (i = 0; i < 4; i++)
    {
        smart_debug("0x%02x,", MTMsg[i]);
    }

    /* print MT Message Data */
    dataLen = MTMsg[1];

    for (i = 0; i < dataLen; i++)
    {
        smart_debug("0x%02x,", (unsigned char)MTMsg[ 4 + i]);
    }

    /* print MT Message FCS */
    smart_debug("0x%02x,", MTMsg[i + 4]);

    return;
}



int assembleMTMsg(unsigned char *sendBuf, int dataLen, char cmd1, char cmd2, unsigned char *data)
{

    sendBuf[0] = MT_UART_SOF;
    sendBuf[1] = dataLen;
    sendBuf[2] = cmd1;
    sendBuf[3] = cmd2;
    memcpy(&sendBuf[4], data, dataLen);
    sendBuf[4 + dataLen] = MT_UartCalcFCS(sendBuf+1, MSG_HEAD_LEN + dataLen);

    printMTMsg(sendBuf);

    return 0;
}
