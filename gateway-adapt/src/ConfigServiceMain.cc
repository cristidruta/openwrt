/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <stdio.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/config/ConfigService.h>
#include <alljoyn/AboutIconObj.h>
#include <SrpKeyXListener.h>
#include <CommonBusListener.h>
#include <CommonSampleUtil.h>
#include <sqlite3.h>

#include "AboutDataStore.h"
#include <alljoyn/AboutObj.h>
#include "ConfigServiceListenerImpl.h"
#include "OptParser.h"
#include <alljoyn/services_common/LogModulesNames.h>

#define DEFAULTPASSCODE "000000"
#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

#define CHECK_RETURN(x) if ((status = x) != ER_OK) { return status; }

/* notification add by gaojing start */
#include <iostream>
#include <sstream>
#include <cstdio>
#include <alljoyn/PasswordManager.h>
#include <alljoyn/notification/NotificationService.h>
#include <alljoyn/notification/NotificationSender.h>
#include <alljoyn/notification/NotificationEnums.h>
#include <alljoyn/notification/NotificationText.h>
#include <alljoyn/notification/RichAudioUrl.h>
#include <alljoyn/notification/Notification.h>
#include <alljoyn/services_common/GuidUtil.h>
#include <errno.h>
#include <sched.h>
#include "share.h"
#include "log.h"

#define MAX_CONFNAME_LEN 128 

extern "C" 
{
    extern char *getOnlineStatusByDevSn(char *manufacture, char *deviceSn, char *onlineStatus, char *deviceType);
    extern int ubusInit(void);
    extern int parseDevOnlineBuf(char *jsonBuf, char *manufacture, char *moduleNumber, char *deviceType, char *devSn, char *transportType);
    extern int convertFromJsonToStr(char *json, char *str);
    extern int find_device_table_callback(void *NotUsed, int argc, char **argv, char **azColName);
    extern sqlite3 *db;
    extern int needCreateFlag;
}
int socketRecvDevBuf(int socketFd, char *recvBuf);
int socketMonitorDev(int socketFd, struct sockaddr_in addr);

#define DEV_ONLINE_PORT 6666 

#define SERVICE_PORT 900

using namespace ajn;
using namespace services;
using namespace qcc;


NotificationService* prodService = 0;
NotificationSender* Sender = 0;

/* notification add by gaojing end */
typedef struct ConfData{
    struct ConfData *next;
    char confName[MAX_CONFNAME_LEN];
}ConfigData;

struct ConfData *confDataHead = NULL;
struct ConfData *confDataTail = NULL;
//sqlite3 *mdb = NULL;

/** static variables need for sample */
static BusAttachment* msgBus = NULL;

static SrpKeyXListener* keyListener = NULL;

static CommonBusListener* busListener = NULL;

static SessionPort servicePort = 900;

static volatile sig_atomic_t s_interrupt = false;

static volatile sig_atomic_t s_restart = false;

static struct ConfData* SearchConfData(char *confName){

    struct ConfData *conf = confDataHead;

    while (conf != NULL) {
        if (strcmp(conf->confName, confName) == 0) {
            return conf;
        }
        conf = conf->next;
    }

    return NULL;
}


static QStatus AddConfData(char *confName) {

    struct ConfData *conf = new ConfData;

    conf->next = NULL;
    memset(conf->confName, 0, sizeof(conf->confName));
    strncpy(conf->confName, confName, MAX_CONFNAME_LEN); 

    if (confDataTail == NULL) {
        confDataTail = conf;
        confDataHead = confDataTail;
    } else {
        confDataTail->next = conf;
        confDataTail = conf;
    }
    return ER_OK;
}

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

static void daemonDisconnectCB()
{
    s_restart = true;
}

static void cleanup() {
#if 0
    if (AboutObjApi::getInstance()) {
        AboutObjApi::DestroyInstance();
    }

    if (configService) {
        delete configService;
        configService = NULL;
    }

    if (configServiceListener) {
        delete configServiceListener;
        configServiceListener = NULL;
    }

    if (keyListener) {
        delete keyListener;
        keyListener = NULL;
    }

    if (busListener) {
        msgBus->UnregisterBusListener(*busListener);
        delete busListener;
        busListener = NULL;
    }

    if (aboutIconObj) {
        delete aboutIconObj;
        aboutIconObj = NULL;
    }

    if (icon) {
        delete icon;
        icon = NULL;
    }

    if (aboutDataStore) {
        delete aboutDataStore;
        aboutDataStore = NULL;
    }

    if (aboutObj) {
        delete aboutObj;
        aboutObj = NULL;
    }
    /* Clean up msg bus */
    if (msgBus) {
        delete msgBus;
        msgBus = NULL;
    }
#endif
}

#if 0
void readPassword(qcc::String& passCode) {

    ajn::MsgArg*argPasscode;
    char*tmp;
    aboutDataStore->GetField("Passcode", argPasscode);
    argPasscode->Get("s", &tmp);
    passCode = tmp;
    return;
}
#endif

void WaitForSigInt(void) {
    while (s_interrupt == false && s_restart == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

//int sqliteCallBack(void *
int socketMonitorDev(int socketFd, struct sockaddr_in addr)
{
    int ret = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEV_ONLINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socketFd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (socketFd == -1) {
        std::cout << "socket error\r\n" << std::endl;
        return 1;
    }   
    std::cout << "create socket ok!" <<std::endl;

    ret = bind(socketFd, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1){  
        std::cout << "bind error \r\n" << std::endl;  
        return -1;  
    }   
    std::cout << "bind to port: " << DEV_ONLINE_PORT << std::endl;
}

int socketRecvDevBuf(int socketFd, char *recvBuf)
{
    int recvBufLen = 0;

    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(client_addr);

    recvBufLen = recvfrom(socketFd, recvBuf, MAX_DEVDATA_LEN, 0,(struct sockaddr*)&client_addr, &client_addr_length);
    //std::cout << "recv from client addr: " << inet_ntoa(client_addr.sin_addr) << std::endl;
    return recvBufLen;
}


int main(int argc, char**argv, char**envArg) {

#ifdef HIGHEST_PRIORITY
    struct sched_param adapt_param;
    int policy;
    int max_priority, min_priority;

    policy = sched_getscheduler(0);
    adapt_debug("use schedule plicy[%d]", policy);
    sched_getparam(0, &adapt_param);
    adapt_debug("schedule priority[%d]", adapt_param.sched_priority);
    sched_get_priority_max(policy);
    sched_get_priority_min(policy);


    adapt_param.sched_priority = 5;
    if (sched_setscheduler(0, SCHED_RR, &adapt_param) == -1)
    {
        adapt_debug("set fxagent schedule failed : [%d]", errno);
        return SERVICE_CONFIG_ERROR;
    }
#endif

    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;
    QCC_SetLogLevels("ALLJOYN_ABOUT_SERVICE=7;");
    QCC_SetLogLevels("ALLJOYN_ABOUT_ICON_SERVICE=7;");
    QCC_SetDebugLevel(logModules::CONFIG_MODULE_LOG_NAME, logModules::ALL_LOG_LEVELS);

    // Initialize Service object
    prodService = NotificationService::getInstance(); //notification add by gaojing


    OptParser opts(argc, argv);
    OptParser::ParseResultCode parseCode(opts.ParseResult());
    switch (parseCode) {
        case OptParser::PR_OK:
            break;

        case OptParser::PR_EXIT_NO_ERROR:
            return SERVICE_EXIT_OK;

        default:
            return SERVICE_OPTION_ERROR;
    }

    std::cout << "using port " << servicePort << std::endl;

    if (!opts.GetConfigFile().empty()) {
        std::cout << "using Config-file " << opts.GetConfigFile().c_str() << std::endl;
    }

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    //set Daemon password only for bundled app
#ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
#endif

start:
    std::cout << "Initializing application." << std::endl;



    /* Create message bus */
    keyListener = new SrpKeyXListener();
    keyListener->setPassCode(DEFAULTPASSCODE);
    //keyListener->setGetPassCode(readPassword);

    /* Connect to the daemon */
    uint16_t retry = 0;
    do {
        msgBus = CommonSampleUtil::prepareBusAttachment(keyListener);
        if (msgBus == NULL) {
            std::cout << "Could not initialize BusAttachment. Retrying" << std::endl;
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
            retry++;
        }
    } while (msgBus == NULL && retry != 180 && !s_interrupt);

    if (msgBus == NULL) {
        std::cout << "Could not initialize BusAttachment." << std::endl;
        cleanup();
        return 1;
    }

    busListener = new CommonBusListener(msgBus, daemonDisconnectCB);
    //busListener->setSessionPort(servicePort);

    /* gaojing: need to add smart device online event here */

    int ret = ER_OK;

    /*create table and save the info */
    ret = sqlite3_open(DEVICE_DATABASE_PATH, &db);
    if (ret != SQLITE_OK) {
        adapt_error("open sqlite3 %s, error[%s]", DEVICE_DATABASE_PATH, sqlite3_errmsg(db));
        //sqlite3_close(db);
        return 1;
    }

    adapt_debug("sqlite open succ");

    /* create table */ 
    const char *sqlQueryTable = "select * from sqlite_master where type='table' and name='deviceinfo';";
    char *errMsg = NULL;
    ret = sqlite3_exec(db, sqlQueryTable, find_device_table_callback, 0, &errMsg);
    if (ret != SQLITE_OK) { 
        adapt_error("query table deviceinfo Error: %s\n", errMsg);
        sqlite3_free(errMsg);
        //return 1;
    }

    adapt_debug("ret = %d ", ret);

    if (needCreateFlag == TRUE) {
        adapt_debug("sqlite create table deviceinfo ");
        const char *sqlCreateTable = "CREATE TABLE deviceinfo(interface TEXT PRIMARY KEY ASC, \
                                      objpath TEXT, platformid TEXT, manufacture TEXT, modulenumber TEXT, \
                                      devicetype TEXT, deviceid TEXT, devicesn TEXT, STATUS TEXT, devData TEXT);";

        ret = sqlite3_exec(db, sqlCreateTable, NULL, 0, &errMsg);
        if (ret != SQLITE_OK) { 
            adapt_error("create table deviceinfo Error: %s\n",  sqlite3_errmsg(db));
            sqlite3_free(errMsg);
        }
        adapt_debug("sqlite create table deviceinfo succeed ");
    }

    adapt_debug("set syn mode to off");
    const char *sqlSynOff = "PRAGMA synchronous = OFF;";
    ret = sqlite3_exec(db, sqlSynOff,  NULL, 0, &errMsg);
    if (ret != SQLITE_OK) {
            adapt_error("Set Syn mode Failed: %s\n",  sqlite3_errmsg(db));
            sqlite3_free(errMsg);
    } else {
        adapt_debug("set syn mode succ");
    }

#if 0
    if (ret == SQLITE_OK) {
        std::cout << "deviceinfo table already existed" << std::endl;
    }
    else if(ret == SQLITE_NOTFOUND) {
        std::cout << "sqlite create table deviceinfo " << std::endl;
        const char *sqlCreateTable = "CREATE TABLE deviceinfo(interface TEXT PRIMARY KEY ASC, \
                                      objpath TEXT, platformid TEXT, manufacture TEXT, modulenumber TEXT, \
                                      devicetype TEXT, deviceid TEXT, devicesn TEXT, STATUS TEXT, devData TEXT)";

        ret = sqlite3_exec(db, sqlCreateTable, NULL, 0, &errMsg);
        if (ret != ER_OK) { 
            fprintf(stderr,"create table deviceinfo Error: %s\n", sqlite3_errmsg(db));
            sqlite3_free(errMsg);
            //return 1;
        }
        std::cout << "sqlite create table deviceinfo succeed " << std::endl;
    }
    else {
        adapt_error("query table deviceinfo Error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }


    //sqlite3_close(db);
    /* open the database in memory, and load DEVICE_DATABASE_PATH database to memory */
    ret = sqlite3_open(":memory:", &mdb);
    if (ret != ER_OK) {
        adapt_error("open memory database error[%s]", sqlite3_errmsg(mdb));
        return 1;
    }

    sqlite3_backup *bp;

    bp = sqlite3_backup_init(mdb,"main", db, "main");
    if (bp != NULL) {
        (void)sqlite3_backup_step(bp, -1);
        (void)sqlite3_backup_finish(bp);
    }
#endif

    int socketFd;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(DEV_ONLINE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socketFd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (socketFd == -1) {
        std::cout << "socket error\r\n" << std::endl;
        return 1;
    }   
    std::cout << "create socket ok!" <<std::endl;

    ret = bind(socketFd, (sockaddr*)&addr, sizeof(addr));
    if (ret == -1){  
        std::cout << "bind error \r\n" << std::endl;  
        return -1;  
    }   
    std::cout << "bind to port: " << DEV_ONLINE_PORT << std::endl;
    //socketMonitorDev(socketFd, addr);
    ubusInit();

    int count = 0;
    int i = 0;
    //while (s_interrupt == false && s_restart == false) {
    while (1) {

        //printf("this is the %d time to enter while loop\n", count);
        adapt_debug("this is the %d time to enter while loop\n", count);
        count ++;

        int recvBufLen = 0;
        int parseBufStatus = 0;
        char devDataJson[MAX_DEVDATA_LEN] = {0};
        recvBufLen = socketRecvDevBuf(socketFd, devDataJson);

        if(recvBufLen > 0)
        {
            //printf("recvBufLen = %d, devDataJson = %s\n", recvBufLen, devDataJson);
            adapt_debug("recvBufLen = %d, devDataJson = %s\n", recvBufLen, devDataJson);
            char manufacture[MAX_MANUFACTURE_LEN] = {0};
            char moduleNumber[MAX_MODULENUMBER_LEN] = {0};
            char deviceSn[MAX_DEVICESN_LEN] = {0};
            char deviceType[MAX_DEVICETYPE_LEN] = {0};
            char transportType[MAX_TRANSPORT_LEN] = {0};
            parseBufStatus = parseDevOnlineBuf(devDataJson, manufacture, moduleNumber, deviceType, deviceSn, transportType);

            if(parseBufStatus == 0)
            {
                /* to do: 
                 * need to add a func to check if the smart device is already online, then to decide if need to start the service or not
                 * according to deviceSn to query the database, if online STATUS is already on, then no need to start the service;
                 * if online STATUS is already off, then need to start the service and update deviceinfo;
                 * if no this device record, need to start the service and insert into deviceinfo
                 * when device is online, configService should be started
                 * when device id offline, configService should be stopped
                 * */
                char onlineStatus[MAX_ONLINESTATUS_LEN] = {0};

                getOnlineStatusByDevSn(manufacture, deviceSn, onlineStatus, deviceType);

                //printf("onlineStatus = %s\n", onlineStatus);
                adapt_debug("onlineStatus = %s\n", onlineStatus);

                if((0 == strcmp(onlineStatus,"")) || (0 == strcmp(onlineStatus, "off")))
                {
                    /* 1st online, need to do all the things */

                    char factoryConfigFileName[MAX_CONFNAME_LEN] = {0};
                    char configFileName[MAX_CONFNAME_LEN] = {0};
                    snprintf(factoryConfigFileName, sizeof(factoryConfigFileName), 
                            "/var/Factory%s%s%s.conf", manufacture, moduleNumber, deviceSn);
                    snprintf(configFileName, sizeof(configFileName), "/var/%s%s%s.conf", manufacture, moduleNumber, deviceSn);


                    qcc::String factoryConfigFile(factoryConfigFileName); 
                    qcc::String configFile(configFileName); 
                    /* above is to snprintf configfielName*/

                    /* if device have been online, don't setup again */
                    if (SearchConfData(configFileName) != NULL)
                    {
                        continue;
                    }

                    if(0 == strcmp(onlineStatus, ""))
                    {
                        char cmd[512] = {0};
                        snprintf(cmd, sizeof(cmd), "cd /usr/bin && cp %s%s.conf %s && cp %s%s.conf %s", 
                                manufacture, moduleNumber, configFileName, manufacture, moduleNumber, factoryConfigFileName);
                        //printf("cmd = %s\n", cmd);
                        adapt_debug("cmd = %s\n", cmd);
                        system(cmd);
                    }

                    static ConfigService* configService = NULL;
                    static AboutIcon* icon = NULL;
                    static AboutIconObj* aboutIconObj = NULL;
                    static AboutDataStore* aboutDataStore = NULL;
                    static AboutObj* aboutObj = NULL;
                    static ConfigServiceListenerImpl* configServiceListener = NULL;

                    aboutDataStore = new AboutDataStore(factoryConfigFile.c_str(), configFile.c_str());
                    aboutDataStore->Initialize();
                    if (!opts.GetAppId().empty()) {
                        std::cout << "using appID " << opts.GetAppId().c_str() << std::endl;
                        aboutDataStore->SetAppId(opts.GetAppId().c_str());
                    }

                    if (status != ER_OK) {
                        std::cout << "Could not fill aboutDataStore." << std::endl;
                        cleanup();
                        return 1;
                    }
                    servicePort = servicePort + 1;
                    busListener->setSessionPort(servicePort); //add by gaojing
                    aboutObj = new ajn::AboutObj(*msgBus, BusObject::ANNOUNCED);
                    status = CommonSampleUtil::prepareAboutService(msgBus, dynamic_cast<AboutData*>(aboutDataStore), aboutObj, busListener, servicePort);
                    if (status != ER_OK) {
                        std::cout << "Could not PrepareAboutService." << std::endl;
                        std::cout << "Could not PrepareAboutService: " << QCC_StatusText(status)  << std::endl;
                        cleanup();
                        return 1;
                    }

                    AboutObjApi* aboutObjApi = AboutObjApi::getInstance();
                    if (!aboutObjApi) {
                        std::cout << "Could not set up the AboutService." << std::endl;
                        cleanup();
                        return 1;
                    }
#if 0
                    /* notification add by gaojing start */
                    NotificationMessageType messageType = NotificationMessageType(INFO);

                    Sender = prodService->initSend(msgBus, dynamic_cast<AboutData*>(aboutDataStore));
                    NotificationText textToSend1("en", "The fridge door is open");

                    std::vector<NotificationText> vecMessages;
                    vecMessages.push_back(textToSend1);
                    Notification notification(messageType, vecMessages);
                    status = Sender->send(notification, 30);
                    if (status != ER_OK) {
                        std::cout << "Could not send the message successfully" << std::endl;
                    } else {
                        std::cout << "Notification sent with ttl of 30ms " << std::endl;
                    }
                    /* notification add by gaojing end */

                    /* notification add by gaojing start */
                    NotificationMessageType messageType1 = NotificationMessageType(INFO);

                    NotificationText textToSend11("en", "{\"type\":\"set\", \"serial\":100, \"user_id\":\"testaccount\", \"device_id\":\"bluetooth_gh_99f35e5ef2ff_190e44bbd5e76a4c\",\"device_type\": \"led\",\"config\": {\"power_switch\": \"on\", \"color_rgb\": \"30\"}}");

                    std::vector<NotificationText> vecMessages1;
                    vecMessages1.push_back(textToSend11);
                    Notification notification1(messageType1, vecMessages1);
                    status = Sender->send(notification1, 30);
                    if (status != ER_OK) {
                        std::cout << "Could not send the message successfully" << std::endl;
                    } else {
                        std::cout << "Notification sent with ttl of 30ms" << std::endl;
                    }
                    /* notification add by gaojing end */
#endif


                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    //aboutIconService

                    uint8_t aboutIconContent[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0A,
                        0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02, 0x50, 0x58, 0xEA, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xAF,
                        0xC8, 0x37, 0x05, 0x8A, 0xE9, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64,
                        0x6F, 0x62, 0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61, 0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x49, 0x44,
                        0x41, 0x54, 0x78, 0xDA, 0x62, 0xFC, 0x3F, 0x95, 0x9F, 0x01, 0x37, 0x60, 0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34, 0x40, 0x80, 0x01, 0x00, 0x06, 0x7C,
                        0x01, 0xB7, 0xED, 0x4B, 0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82 };

                    qcc::String mimeType("image/png");
                    icon = new ajn::AboutIcon();
                    status = icon->SetContent(mimeType.c_str(), aboutIconContent, sizeof(aboutIconContent) / sizeof(*aboutIconContent));
                    if (ER_OK != status) {
                        //printf("Failed to setup the AboutIcon.\n");
                        adapt_error("Failed to setup the AboutIcon.\n");
                    }

                    aboutIconObj = new ajn::AboutIconObj(*msgBus, *icon);

                    /* gaojing: need to define smart device's interfaceName and objectPath*/
                    char ifName[MAX_INTERFACE_LEN] = {0};
                    char objectPath[MAX_OBJECTPATH_LEN] = {0};
                    snprintf(ifName, sizeof(ifName), "%s.%s.%s.Config", manufacture, moduleNumber, deviceSn);
                    snprintf(objectPath, sizeof(objectPath), "/%s/Config", deviceSn);

                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    //ConfigService
                    std::cout << "interface: " << ifName << std::endl;
                    std::cout << "object path :" << objectPath << std::endl;

                    configServiceListener = new ConfigServiceListenerImpl(*aboutDataStore, *msgBus, *busListener);
                    configService = new ConfigService(*msgBus, *aboutDataStore, *configServiceListener, objectPath);

                    status = configService->Register(ifName);
                    if (status != ER_OK) {
                        std::cout << "Could not register the ConfigService." << std::endl;
                        cleanup();
                        return 1;
                    }

                    status = msgBus->RegisterBusObject(*configService);
                    if (status != ER_OK) {
                        std::cout << "Could not register the ConfigService BusObject." << std::endl;
                        cleanup();
                        return 1;
                    }

                    std::cout << "register object succ " << std::endl;

                    InterfaceDescription* intf = const_cast<InterfaceDescription*>(msgBus->GetInterface(ifName));

                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    if (ER_OK == status) {

                        /* save info befor announce */
                        /* gaojing: need to get smart device info such as factory, dev_type and dev_id */
                        {
#if 0 
                            sqlite3 *db;
                            
                            /*create table and save the info */
                            ret = sqlite3_open("/var/device.db", &db);
                            if (ret != ER_OK) {
                                std::cout << "sqlite3_open error !" << std::endl;
                                sqlite3_close(db);
                                return 1;
                            }

                            std::cout << "sqlite open succ" << std::endl;
#endif

                            const int SQL_CMD_LEN = 256 + MAX_DEVDATA_LEN;
                            char sqlCmd[SQL_CMD_LEN];

                            char devDataStr[MAX_DEVDATA_LEN] = {0};
                            convertFromJsonToStr(devDataJson, devDataStr);
                            memset(sqlCmd, 0, sizeof(sqlCmd));
                            snprintf(sqlCmd, sizeof(sqlCmd) - 1, 
                                    "INSERT INTO deviceinfo VALUES(\"%s\", \"%s\", \"\", \"%s\", \"%s\", \"%s\", \"\", \"%s\", \"on\", \'%s\');",
                                    ifName, objectPath, manufacture, moduleNumber , deviceType, deviceSn, devDataStr);

                            adapt_debug("sqlCmd = %s\n", sqlCmd);

                            //sqlite3_exec(db,"begin;",0,0,0);
                            ret = sqlite3_exec(db, sqlCmd, NULL/*sqliteCallback*/, 0, &errMsg);
                            if (ret != ER_OK) {
                                adapt_error("insert value Error: %s\n", sqlite3_errmsg(db));
                                sqlite3_free(errMsg);
                                //return 1;
                            }
                            else
                            {
                                adapt_debug("begin excute sql");
                                //sqlite3_exec(db,"commit;",0,0,0);
                                //std::cout << "insert succeed" << std::endl;
                                adapt_debug("insert succeed");
                            }

                            //sqlite3_close(db);
                        }
                        status = aboutObjApi->Announce();
                    }
                    std::cout << "Announce succ" << std::endl;

                    /* above part is aboutData init, used for offline and 1st online */

                    AddConfData(configFileName);


                }
                else
                {
                    //printf("already online, no need to handle\n");
                    adapt_debug("already online, no need to handle\n");
                }
            }
        } else {
            //printf(" not recv any buf\n");
            adapt_debug(" not recv any buf\n");

        }

        usleep(100 *1000);
    }

    //printf("exit while loop\n");
    adapt_debug("exit while loop\n");
    std::cout << "s_restart = " << s_restart << std::endl;
    std::cout << "s_interrupt = " << s_interrupt << std::endl;
#if 0
    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }
#endif
    cleanup();

    if (s_restart) {
        s_restart = false;
        goto start;
    }

    return 0;
} /* main() */
