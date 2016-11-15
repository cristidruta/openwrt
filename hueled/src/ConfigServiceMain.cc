#include <ctype.h>
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

#define MAX_CONFNAME_LEN 256 

#include "pCtlIntf.h"

extern "C" 
{
    extern int sqlModuleInit();
    extern int getDevStsByDevSn(char *manufacture, char *devSn);
    extern int sqlUpdateDevData(char *manufacture, char *sn, char *intfName, char *objPath, char *moduleNumber, char *devType, char *devData);

#ifdef TCP_PROXY_CTL_INTF
    extern int g_pCtlFd;
    extern int pCtlIntf_init();
    extern int PCtlMsg_receive(int fd, PCtlMsgHeader **buf);
#endif
}
#ifdef IP_DEV_CTL_SERVER
    extern int devMgmt_init();
    extern void devMgmt_loop();
#endif

extern int cloudc_pthread_init();

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

/** static variables need for sample */
static BusAttachment* msgBus = NULL;
static SrpKeyXListener* keyListener = NULL;
static CommonBusListener* busListener = NULL;
static SessionPort servicePort = 900;
static volatile sig_atomic_t s_interrupt = false;
static volatile sig_atomic_t s_restart = false;
static uint8_t appId[]= {0x06, 0x52, 0x02, 0xf9, 0x57, 0xc3, 0x4c, 0x97, 0x84, 0x3f, 0x2f, 0x9c, 0x00, 0x00, 0x00, 0x01};

static struct ConfData* SearchConfData(char *confName)
{
    struct ConfData *conf = confDataHead;

    while (conf != NULL) {
        if (strcmp(conf->confName, confName) == 0) {
            return conf;
        }
        conf = conf->next;
    }

    return NULL;
}

static QStatus AddConfData(char *confName)
{
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
}

void WaitForSigInt(void) {
    while (s_interrupt == false && s_restart == false) {
        usleep(100 * 1000);
    }
}

int devOnlineHandle(char *manufactureSN, char *name, char *power, char *bright, char * version)
{
    QStatus status = ER_OK;
    int devStatus=-1;
    char *manufacture="Philips";
   // char *devSn="00:17:88:01:02:31:a5:ed-0b";
    char *devType="Led";
    char *moduleNumber="led";
    char *devDataFmt = "{\"name\":\"%s\", \
\"power\":\"%s\", \
\"bright\":\"%s\", \
\"manufacture\":\"Philips\", \
\"manufactureSN\":\"%s\", \
\"manufactureDataModelId\":\"1\", \
\"deviceType\":\"Led\", \
\"type\":\"Extended color light\", \
\"modelId\":\"LCT007\", \
\"softwareVersion\":\"%s\"}";
    char devData[512]={0};

    snprintf(devData, sizeof(devData),devDataFmt, name, power, bright, manufactureSN, version);
    devStatus=getDevStsByDevSn(manufacture, manufactureSN);
    //if ( 0 == devStatus || -1 == devStatus )
    {
        /* first online, need to do all the things */
        char factoryConfigFileName[MAX_CONFNAME_LEN] = {0};
        char configFileName[MAX_CONFNAME_LEN] = {0};
        char intfName[MAX_INTERFACE_LEN] = {0};
        char objectPath[MAX_OBJECTPATH_LEN] = {0};
        static ConfigService* configService = NULL;
        static AboutIcon* icon = NULL;
        static AboutIconObj* aboutIconObj = NULL;
        static AboutDataStore* aboutDataStore = NULL;
        static AboutObj* aboutObj = NULL;
        static ConfigServiceListenerImpl* configServiceListener = NULL;

        snprintf(factoryConfigFileName, sizeof(factoryConfigFileName), 
                "/var/Factory%s%s%s.conf", manufacture, moduleNumber, manufactureSN);
        snprintf(configFileName, sizeof(configFileName), "/var/%s%s%s.conf", manufacture, moduleNumber, manufactureSN);


        qcc::String factoryConfigFile(factoryConfigFileName); 
        qcc::String configFile(configFileName); 
        /* above is to snprintf configfielName*/

        /* if device have been online, don't setup again */
        if (SearchConfData(configFileName) != NULL)
        {
        }

        //if ( -1 == devStatus )
        {
            char cmd[1024] = {0};
            snprintf(cmd, sizeof(cmd), "cp /usr/bin/%s%s.conf %s && cp /usr/bin/%s%s.conf %s", 
                    manufacture, moduleNumber, configFileName, manufacture, moduleNumber, factoryConfigFileName);
            adapt_debug("cmd=%s\n", cmd);
            system(cmd);
        }

        aboutDataStore = new AboutDataStore(factoryConfigFile.c_str(), configFile.c_str());
        aboutDataStore->Initialize();

        status = aboutDataStore->SetAppId(appId, 16);
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


        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //aboutIconService
        uint8_t aboutIconContent[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
                                      0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 
                                      0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 
                                      0x08, 0x02, 0x00, 0x00, 0x00, 0x02, 0x50, 0x58, 
                                      0xEA, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 
                                      0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A, 
                                      0xE9, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 
                                      0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 
                                      0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 
                                      0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61, 
                                      0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 
                                      0x00, 0x18, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 
                                      0x62, 0xFC, 0x3F, 0x95, 0x9F, 0x01, 0x37, 0x60, 
                                      0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34, 0x40, 0x80, 
                                      0x01, 0x00, 0x06, 0x7C, 0x01, 0xB7, 0xED, 0x4B, 
                                      0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 
                                      0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

        qcc::String mimeType("image/png");
        icon = new ajn::AboutIcon();
        status = icon->SetContent(mimeType.c_str(), aboutIconContent, sizeof(aboutIconContent) / sizeof(*aboutIconContent));
        if (ER_OK != status) {
            adapt_error("Failed to setup the AboutIcon.\n");
        }

        aboutIconObj = new ajn::AboutIconObj(*msgBus, *icon);

        /* gaojing: need to define smart device's interfaceName and objectPath*/
        if (strcmp(name,"lamp 1")==0){
            snprintf(intfName, sizeof(intfName), "%s.%s.001788010231a5ed0b.Config", manufacture, moduleNumber);

            snprintf(objectPath, sizeof(objectPath), "/001788010231a5ed0b/Config");
        }
        else{
            snprintf(intfName, sizeof(intfName), "%s.%s.001788010231d62c0b.Config", manufacture, moduleNumber);

            snprintf(objectPath, sizeof(objectPath), "/001788010231d62c0b/Config");
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //ConfigService
        std::cout << "interface: " << intfName << std::endl;
        std::cout << "object path :" << objectPath << std::endl;

        configServiceListener = new ConfigServiceListenerImpl(*aboutDataStore, *msgBus, *busListener);
        configService = new ConfigService(*msgBus, *aboutDataStore, *configServiceListener, objectPath);

        status = configService->Register(intfName);
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

        InterfaceDescription* intf = const_cast<InterfaceDescription*>(msgBus->GetInterface(intfName));

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        if (1 != devStatus) {
            /* save info befor announce */
            if (sqlUpdateDevData(manufacture,
                        manufactureSN, 
                        intfName, 
                        objectPath, 
                        moduleNumber , 
                        devType, 
                        devData) < 0) 
            {
                adapt_error("sqlUpdateDevData failed!");
            }
            status = aboutObjApi->Announce();
        }
        std::cout << "Announce succ" << std::endl;

        /* above part is aboutData init, used for offline and 1st online */
        AddConfData(configFileName);
    }
}

int setHighPriority()
{
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
    return 0;
}

#ifdef TCP_PROXY_CTL_INTF
void readMsgFromPCtl(void)
{
    PCtlMsgHeader *msg=NULL;
    int ret=-1;

    while ((ret=PCtlMsg_receive(g_pCtlFd, &msg)) == 0)
    {
        switch(msg->type)
        {
            case PCTL_MSG_DEV_ONLINE:
                devOnlineHandle();
                break;

            default:
                adapt_error("unrecognized msg 0x%x\r\n", msg->type);
                break;
        }

        free(msg);
        msg=NULL;
    }

    return;
}

int pCtlIntf_loop()
{
    int max_fd=g_pCtlFd;
    int nready;
    int ret=0;
    fd_set rset;

    /* detach from the terminal so we don't catch the user typing control-c.
    if (setsid() == -1)
    {
        printf("Could not detach from terminal");
    } */

    for (;;) {
        FD_ZERO(&rset);
        FD_SET(g_pCtlFd,&rset);

        nready = select(max_fd+1, &rset, NULL, NULL, NULL);
        if (nready == -1)
        {
            adapt_error("error on select, errno=%d\r\n", errno);
            usleep(100);
            continue;
        }

        if (FD_ISSET(g_pCtlFd, &rset)) {
            readMsgFromPCtl();
        }
    }

    return 0;
}
#endif

int main(int argc, char**argv, char**envArg)
{
    QStatus status = ER_OK;

    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;
    
    setHighPriority();
    
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
            sleep(1);
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

    if (sqlModuleInit() < 0) {
        adapt_error("sqlModuleInit() failed!");
        return -1;
    }

    
    char *FmanufactureSN = "00:17:88:01:02:31:a5:ed-0b";
    char *SmanufactureSN = "00:17:88:01:02:31:d6:2c-0b";
    char *Fname = "lamp 1";
    char *Sname = "Hue color lamp 1";
    char *Fpower = "ture";
    char *Spower = "ture";
    char *Fbright = "200";
    char *Sbright = "200";
    char *Fversion = "5.50.1.19085";
    char *Sversion = "5.38.1.14919";

    

    devOnlineHandle(FmanufactureSN, Fname, Fpower, Fbright,Fversion);
    devOnlineHandle(SmanufactureSN, Sname, Spower, Sbright,Sversion);

    WaitForSigInt();
#ifdef TCP_PROXY_CTL_INTF
    if (pCtlIntf_init() < 0) {
        adapt_error("pCtlIntf_init() failed!");
        return -1;
    }

    pCtlIntf_loop();
#endif

#ifdef IP_DEV_CTL_SERVER
    if (devMgmt_init() < 0) {
        adapt_error("devMgmt_init() failed!");
        return -1;
    }

    cloudc_pthread_init();
    devMgmt_loop();
#endif

    cleanup();

    if (s_restart) {
        s_restart = false;
        goto start;
    }

    return 0;
} /* main() */
