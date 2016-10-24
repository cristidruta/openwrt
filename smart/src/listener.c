#include "share.h"
#include <termios.h>
#include <unistd.h>


//#define IEEEADDR_LEN 8
#define IEEEADDR_HEX_LEN    17

int devUloopFdCreate();
void monitorIPUloopFd(struct uloop_fd *u, unsigned int events);
void monitorZigbeeUloopFd(struct uloop_fd *u, unsigned int events);
void monitorBluetoothUloopFd(struct uloop_fd *u, unsigned int events);
void recvConfigMessage(struct uloop_fd *u, unsigned int events);
void printDevList(void);
short getDevNWKAddrFromList(char *devIEEEAddr);

struct uloop_fd configFd = {
    .cb = recvConfigMessage,
    .fd = -1
};


struct uloop_fd ipUloopFd = {
    .cb = monitorIPUloopFd,
    .fd = -1
};


struct uloop_fd zigbeeUloopFd = {
    .cb = monitorZigbeeUloopFd,
    .fd = -1
};

struct uloop_fd bluetoothUloopFd = {
    .cb = monitorBluetoothUloopFd,
    .fd = -1
};

enum {
    ER_FAIL = -1,
    ER_OK = 0
};


typedef struct devAddrList{
    short nwkAddr;
    char IEEEAddrHex[IEEEADDR_HEX_LEN];
    struct devAddrList *next;
}devAddrListNode;

devAddrListNode *devAddrHead = NULL;

void setTtySpeed(int fd, int speed) {

    int status;
    struct termios   Opt;

    tcgetattr(fd, &Opt);
    tcflush(fd, TCIOFLUSH);
    cfsetispeed(&Opt, B115200);
    cfsetospeed(&Opt, B115200);
    Opt.c_cflag |= (CLOCAL | CREAD);


    status = tcsetattr(fd, TCSANOW, &Opt);
    if (status != 0) {
        perror("set terminal speed error!");
    }

    return;
}

int setTtyParity(int fd,int databits,int stopbits,int parity)
{

        struct termios options;

        if  ( tcgetattr( fd,&options)  !=  0) {
                perror("setup serial error!");
                return ER_FAIL;
        }
        options.c_cflag &= ~CSIZE;
        switch (databits)
        {
                case 7:
                        options.c_cflag |= CS7;
                        break;
                case 8:
                        options.c_cflag |= CS8;
                        break;
                default:
                        perror("unsupported data size");
                        return ER_FAIL;
        }
                        
        switch (parity)
        {
                case 'n':
                case 'N':
                        options.c_cflag &= ~PARENB;   /* Clear parity enable */
                        options.c_iflag &= ~INPCK;     /* Enable parity checking */
                        break;
                case 'o':
                case 'O':
                        options.c_cflag |= (PARODD | PARENB); /* ......*/
                        options.c_iflag |= INPCK;             /* Disnable parity checking */
                        break;
                case 'e':
                case 'E':
                        options.c_cflag |= PARENB;     /* Enable parity */
                        options.c_cflag &= ~PARODD;   /* ......*/
                        options.c_iflag |= INPCK;       /* Disnable parity checking */
                        break;
                case 'S':
                case 's':  /*as no parity*/
                        options.c_cflag &= ~PARENB;
                        options.c_cflag &= ~CSTOPB;
                        options.c_iflag |= INPCK;
                        break;
                default:
                        //printf("unsupported parity\r\n");
                        smart_debug("unsupported parity\r\n");
                        return ER_FAIL;
        }

        switch (stopbits)
        {
                case 1:
                        options.c_cflag &= ~CSTOPB;
                        break;
                case 2:
                        options.c_cflag |= CSTOPB;
                        break;
                default:
                        //printf("unsupported stop bits");
                        smart_error("unsupported stop bits");
                        return ER_FAIL;
        }
                        
        /* Set input parity option */
        /*if (parity != 'n')
          options.c_iflag |= INPCK; */

        options.c_cflag &= ~CRTSCTS;/* no hardware flow */

        options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        options.c_oflag  &= ~OPOST;   /*Output*/
        tcflush(fd,TCIFLUSH);
        options.c_cc[VTIME] = 10;
        options.c_cc[VMIN] = 0;
        if (tcsetattr(fd,TCSANOW,&options) != 0)
        {

            perror("setup serial error");
            return ER_FAIL;
        }
        return ER_OK;
}
        


int devZigbeeUloopFdCreate()
{
    const int ZIGBEE_DEV_DEFAULT_SPEED = 115200;
    int speed = ZIGBEE_DEV_DEFAULT_SPEED;
    int dataBits = 8;
    int stopBits = 1;
    char parity = 'N';

    int fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("open tty ACM error!\r\n");
    }

    zigbeeUloopFd.fd = fd;

    setTtySpeed(fd, speed);
    setTtyParity(fd, dataBits, stopBits, parity);
    
    smart_debug("%s[%d]: init ttyACM0 succ ", __func__, __LINE__);
    return ER_OK;
}

int devBluetoothUloopFdCreate()
{
    return ER_OK;
}

int devIPUloopFdCreate()
{
    struct sockaddr_in addr;
    const int DEV_ONLINE_PORT = 7666;
    int ret = 0;

    ipUloopFd.fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (ipUloopFd.fd == -1) {
        perror("socket create error:");
        return 1;
    }   

    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEV_ONLINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(ipUloopFd.fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1){  
        perror("socket bind error:");
        return 1;  
    }   
    
    return 0;
}

int createSocketForConfigMessage()
{
    struct sockaddr_in addr;
    const int SMART_CONFIG_PORT = 16666;
    int ret = 0;

    configFd.fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (configFd.fd == -1) {
        perror("socket create error:");
        return 1;
    }   

    addr.sin_family = AF_INET;
    addr.sin_port = htons(SMART_CONFIG_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret = bind(configFd.fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1){  
        perror("socket bind error:");
        return 1;  
    }   
    
    return 0;
}

void recvConfigMessage(struct uloop_fd *u, unsigned int events)
{
    char recvBuf[RECV_MAX_BUF_LEN] = {0};
    int recvBufLen = 0;
    struct json_object* recvJsonObj = NULL;
    struct json_object* typeJsonObj = NULL;
    const char *type = NULL;


    struct json_object* devSnJsonObj= NULL;
    const char *devSn = NULL;

    struct json_object* configJsonObj= NULL;
    const char *config = NULL;
    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);
    short nwkAddr;
    unsigned char *data = NULL;
    int len = 0;
    unsigned char sendBuf[SEND_MAX_BUF_LEN] = {0};
    int ret = 0;

    smart_debug("%s[%d]: Enter ", __func__, __LINE__);
    recvBufLen = recvfrom(u->fd, recvBuf, RECV_MAX_BUF_LEN, 0,(struct sockaddr*)&client_addr, &client_addr_length);
    smart_debug("recvBufLen = %d, recvBuf = %s\n", recvBufLen, recvBuf);
    recvJsonObj = json_tokener_parse(recvBuf);
    typeJsonObj = json_object_object_get(recvJsonObj, "transportType");
    type = json_object_get_string(typeJsonObj);

    devSnJsonObj = json_object_object_get(recvJsonObj, "device_sn");
    devSn = json_object_get_string(devSnJsonObj);

    configJsonObj = json_object_object_get(recvJsonObj, "config");
    config = json_object_get_string(configJsonObj);

    if (strcmp(type, "zigbee") == 0)
    {
        nwkAddr = getDevNWKAddrFromList(devSn); 
        if (nwkAddr == -1)
        {
            smart_error("device isn't exist\r\n");
            printDevList();
            return; 
        }
    
        len = sizeof(nwkAddr) + strlen(config) + 1 + 2;
        data = (unsigned char *)malloc(len);

        data[0] = sizeof(nwkAddr);
        memcpy(data + 1, &nwkAddr, sizeof(nwkAddr));
        data[sizeof(nwkAddr) + 1] = strlen(config) + 1;
        memcpy(data + sizeof(nwkAddr) + 2, config, strlen(config) + 1);

        assembleMTMsg(sendBuf, len, 0x29, 0x02, data);

        ret = write(zigbeeUloopFd.fd, sendBuf, len + 5);
        if (ret < 0) 
        {
            smart_error(" write zigbee ACM tty error!");
        }

        free(data);
    }

    return;

}

void monitorIPUloopFd(struct uloop_fd *u, unsigned int events)
{
    char recvBuf[RECV_MAX_BUF_LEN] = {0};
    int recvBufLen = 0;

    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);

    smart_debug("%s[%d]: Enter ", __func__, __LINE__);
    recvBufLen = recvfrom(u->fd, recvBuf, RECV_MAX_BUF_LEN, 0,(struct sockaddr*)&client_addr, &client_addr_length);
    //printf("recvBufLen = %d, recvBuf = %s\n", recvBufLen, recvBuf);
    smart_debug("recvBufLen = %d, recvBuf = %s\n", recvBufLen, recvBuf);
    
    if (recvBufLen > 0)
    { 
        //parseRecvBufFromAdapter(recvBuf);
        sendJsonMsgToAdapt(recvBuf);
    }
    else if (-1 == recvBufLen)
    {
        perror("Receive Data Failed:");
        //close(u->fd);
    }
    else
    {
        smart_debug("%s[%d]: recvBufLen = %d, no need to handle ", __func__, __LINE__, recvBufLen);
    }
    smart_debug("%s[%d]: Exit ", __func__, __LINE__);
}


#define MAX_JSON_STR_LEN 1024


int addDevAddrToList(char *IEEEAddrPtr, short nwkAddr)
{
    devAddrListNode *devAddrNode = NULL;

    devAddrNode = (devAddrListNode *)malloc(sizeof(devAddrListNode));
    if (devAddrNode == NULL)
    {
        smart_error("malloc error\r\n");
        return -1;
    }

    devAddrNode->nwkAddr = nwkAddr;
    strncpy(devAddrNode->IEEEAddrHex, IEEEAddrPtr, IEEEADDR_HEX_LEN);
    devAddrNode->next = NULL;

    if (devAddrHead == NULL)
    {
        devAddrHead = devAddrNode;
    } else {
        devAddrNode->next = devAddrHead;
        devAddrHead = devAddrNode;
    }

    return 0;
}


void printDevList(void)
{
    devAddrListNode *devAddrNode = NULL;

    devAddrNode = devAddrHead;
    while (devAddrNode)
    {
        smart_debug("IEEEAddrHex = %s  nwkAddr = 0x%02x\r\n", devAddrNode->IEEEAddrHex, devAddrNode->nwkAddr);
        devAddrNode = devAddrNode->next;
    }
    return;
}

short getDevNWKAddrFromList(char *devIEEEAddr)
{

    devAddrListNode *devAddrNode = NULL;

    devAddrNode = devAddrHead;
    while (devAddrNode)
    {
        if (strcmp(devIEEEAddr, devAddrNode->IEEEAddrHex) == 0)
            return devAddrNode->nwkAddr;
        else
            devAddrNode = devAddrNode->next;
    }
    return -1;
}




void monitorZigbeeUloopFd(struct uloop_fd *u, unsigned int events)
{
    int len;
    char recvBuf[RECV_MAX_BUF_LEN] = {0};
    int cmd1;
    int cmd2;
    char data[RECV_MAX_BUF_LEN] = {0};
    int dataLen;
    char jsonStr[MAX_JSON_STR_LEN] = {0};
    char IEEEAddr[IEEEADDR_LEN];
    char IEEEAddrHex[IEEEADDR_HEX_LEN];
    short nwkAddr;
    int i;
    
    
    smart_debug("%s[%d]: Enter ", __func__, __LINE__);

    if ((len = read(u->fd, recvBuf, sizeof(recvBuf))) < 0)
    {
        smart_error("read ACM tty error");
        return;
    }

    /* there is a bug which change the data length byte to 0xA, 
     * maybe bug in linux ACM driver, we should fix it later */
    recvBuf[1] = 0xD;

    if(parseZigbeeAnnceMsg(recvBuf, len, IEEEAddr, &nwkAddr) < 0)
    {
        smart_error("parseZigbeeAnnceMsg error \r\n");
        return;
    }
    
    for( i = 0; i < IEEEADDR_LEN; i++)
    {
       sprintf(IEEEAddrHex + i*2,"%02x", (unsigned char)IEEEAddr[i]);
    }

    IEEEAddrHex[IEEEADDR_HEX_LEN] = '\0';

    if (addDevAddrToList(IEEEAddrHex, nwkAddr) < 0)
    {
        smart_error("addDevAddrToList error\r\n");
        printDevList();
        return;
    }
   
    assembleJsonMsg(IEEEAddrHex, jsonStr);

    if (sendJsonMsgToAdapt(jsonStr) < 0)
    {
        smart_error("sendJsonMsgToAdapt error\r\n");
        return;
    }
    
    smart_debug("%s[%d]: Exit ", __func__, __LINE__);
    return;
}

void monitorBluetoothUloopFd(struct uloop_fd *u, unsigned int events)
{

    return;
}


