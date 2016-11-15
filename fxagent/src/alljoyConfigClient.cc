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

#include <signal.h>
#include <set>

#include <alljoyn/config/ConfigClient.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutIconProxy.h>
#include <alljoyn/version.h>
#include <alljoyn/services_common/LogModulesNames.h>
#include "samples_common/SrpKeyXListener.h"
#include "samples_common/AsyncSessionJoiner.h"
#include "samples_common/SessionListenerImpl.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include "NewConfigClient.h"
#include "cloudc_log.h"

/* notification add by gaojing start 2016.06.28 */
#include <algorithm>
#include <iostream>
#include <signal.h>
#include <alljoyn/PasswordManager.h>
#include <sstream>
#include <alljoyn/PasswordManager.h>
#include <alljoyn/notification/NotificationService.h>
#include "common/NotificationReceiverTestImpl.h"
#include <alljoyn/notification/NotificationText.h>
#include <alljoyn/notification/RichAudioUrl.h>
#include <alljoyn/notification/NotificationEnums.h>
#include "samples_common/CommonSampleUtil.h"
#include <alljoyn/notification/Notification.h>
#include <alljoyn/services_common/GuidUtil.h>
#include <alljoyn/services_common/LogModulesNames.h>
#include <string.h>
using namespace qcc;
using namespace ajn;
using namespace services;

#include "share.h"
//#include "parser.h"
#include "proto.h"
// Set application constants
#define DEVICE_NAME "testdeviceName"
#define APP_ID 1234
#define APP_NAME "fxagent"
#define LANG1  "devupg"
//#define TEXT1 "Hello World"
//#define LANG2  "es"
//#define TEXT2 "Hola Mundo"
#define KEY1 "On"
#define VAL1 "Hello"
#define KEY2 "Off"
#define VAL2 "Goodbye"
#define URL1 "http://url1.com"
#define URL2 "http://url2.com"
#define RICH_ICON_URL "http://iconurl.com"
#define CONTROL_PANEL_SERVICE_OBJECT_PATH "/ControlPanel/MyDevice/areYouSure"
#define RICH_ICON_OBJECT_PATH "/icon/objectPath"
#define RICH_AUDIO_OBJECT_PATH "/Audio/objectPath"
#define SERVICE_PORT 900

/* notification add by gaojing end 2016.06.28 */

extern "C" int NotificationSendMain(char *msg_type, int serial, char *deviceid, char *userid, char *ImpPath);
extern "C"
{
#include "parser.h"
#include "share.h"
int cloudc_send_online_buf(char *devData);
void cloudc_get_ipk_name(struct ipk_info *list_head, char *ipk_name, char *node_value);
void cloudc_print_ipk_name_list(struct ipk_info *list_head);
void cloudc_destroy_ipk_name(struct ipk_info *list_head);
void getDevInfoByObj(char *interfaceName, char *objectPath, char *deviceId, char *deviceType);
void getDevDataByObj(char *interfaceName, char *devData);
}

extern "C" void StartAlljoynService();
extern "C" void StopAlljoynService();
extern "C" int configClientMain(char *interfaces, char *matchObjectPath, int msgType, char *devData);
void HandleUpdateConfigMethod(const char *interfaces);
void UpdateConfig(qcc::String const& busName, unsigned int id, const char *interfaceName, const char *objectPath, int msgType, char *devData);
int CheckIfConfigInterfaceOnline(ajn::MsgArg objectDescriptionArg, ajn::MsgArg aboutDataArg, char *interfaceName, char *objectPath, const char *busName, SessionPort port);
int ParseGetAboutDataArg(ajn::MsgArg aboutDataArg, char *devData);
int InteresteForAllConfigInterfaces(const char **interfaces);
int NotificationInit();

unsigned int remoteConfigFlag = 0;
unsigned int callbackFinish = 0;
int ret = 0;
int getMsgType = -1;
char* busname = NULL;
char* objectPath = NULL;
char* interfaceName = NULL;
char *getDevData = NULL;


NotificationService* prodService = 0;
NotificationSender* Sender = 0;
 //    BusAttachment* bus = 0;
AboutData* aboutData = NULL;
AboutObj* aboutObj = NULL;
CommonBusListener*  busListener = 0;
     
//char g_busName[256]={0};
typedef enum 
{
    Get,
    Set,
    Notify
}MsgType;

using namespace ajn;
using namespace services;

#define INITIAL_PASSCODE "000000"
#define NEW_PASSCODE "12345678"

//SessionId g_sessionId;
//SessionPort g_port;

typedef struct interfaceNamelist{
    char interfaceName[MAX_INTERFACE_LEN];
    struct interfaceNamelist *next;
}interfaceNamelist;
typedef struct sessionInfo {
    struct interfaceNamelist *interface_name_list;
    char busName[MAX_BUSNAME_LEN];
    SessionPort port;
    SessionId sessionId;
    struct sessionInfo *next;
}sessionInfo;

sessionInfo *sessionInfoList = NULL;


static BusAttachment* busAttachment = 0;
static SrpKeyXListener* srpKeyXListener = 0;
static std::set<qcc::String> handledAnnouncements;

static volatile sig_atomic_t s_interrupt = false;
static volatile sig_atomic_t s_stopped = false;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

/* notification add by gaojing start */
NotificationService* conService = 0;
NotificationReceiverTestImpl* Receiver = 0;
static volatile sig_atomic_t s_interrupt_notify = false;

sessionInfo *findSessionInfo(char *interfaceName)
{
    sessionInfo *node = NULL;

    cloudc_debug("Enter findSessionInfo");
    node = sessionInfoList;

    while(node)
    {
        interfaceNamelist *list = NULL;
        list = node->interface_name_list;
        while(list)
        {

            if (strcmp(interfaceName, list->interfaceName) == 0)

            {
                cloudc_debug("find session info[%s]", interfaceName);
                return node;
            }
            list = list->next;
        }

        node = node->next;
    }


    cloudc_debug("don't find session info[%s]", interfaceName);
    cloudc_debug("Exit findSessionInfo");
    return NULL;
}

sessionInfo *findBusName(const char *busName)
{
    sessionInfo *node = NULL;

    cloudc_debug("Enter findBusName!");
    node = sessionInfoList;

    while(node)
    {
        if (strcmp(busName, node->busName) == 0)
        {
            cloudc_debug("find session info[%s]", busName);
            return node;
        }

        node = node->next;
    }


    cloudc_debug("don't find busName[%s]", busName);
    cloudc_debug("Exit findBusName");
    return NULL;
}

void insertSessionInfo(char *interfaceName, const char *busName, SessionPort port)
{
    sessionInfo *node = NULL;

    cloudc_debug("Enter insertSessionInfo");
    if (findSessionInfo(interfaceName) == NULL)
    {
       
        /* insert new session info to list head */
        if (findBusName(busName) == NULL)
        { 
            node = (sessionInfo *)malloc(sizeof(sessionInfo));
            interfaceNamelist *list = NULL;
            list = (interfaceNamelist *)malloc(sizeof(interfaceNamelist));
            if (node != NULL && list != NULL)
            {
                strncpy(list->interfaceName, interfaceName, sizeof(list->interfaceName));
                strncpy(node->busName, busName, sizeof(node->busName));
                node->port = port;
                list->next = NULL;
                node->interface_name_list = list;
                node->next=NULL;
            }
            else
            {
                cloudc_debug("malloc session node failed");
                return;
            }
            node->next = sessionInfoList;
            sessionInfoList = node;

        

            QStatus status = ER_OK;
            SessionId sessionId;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                    SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            status = busAttachment->JoinSession(node->busName, node->port, NULL, sessionId, opts);
            if(status == ER_OK)
            {
                node->sessionId = sessionId;
                //g_sessionId = sessionId;
                //strcpy(g_busName, node->busName);
                std::cout << "JoinSession Success, sessionId = "<<sessionId<<"" << std::endl;
                std::cout << "objectPath = " << objectPath << std::endl;
                std::cout << "interfaceName = " << interfaceName << std::endl;
            }
            else
            {
                std::cout << "WARNING - JoinSession failed: " << QCC_StatusText(status) << std::endl;
            }
        }
        else
        {
            node = (sessionInfo *)malloc(sizeof(sessionInfo));
            interfaceNamelist *list, *tmp = NULL;
            list = (interfaceNamelist *)malloc(sizeof(interfaceNamelist));
            tmp = (interfaceNamelist *)malloc(sizeof(interfaceNamelist));

            if (node != NULL && tmp != NULL && tmp != NULL)
            {
                node = findBusName(busName);
                strncpy(tmp->interfaceName, interfaceName, sizeof(tmp->interfaceName));
                list = node->interface_name_list; 
                while (list->next)
                {
                    list = list->next;
                }

                tmp->next = NULL;
                list->next = tmp;
                std::cout << "don't need to JoinSession" << std::endl;
            }
        }
    }
    else
    {
        cloudc_debug("interface[%s] had exsited", interfaceName);
    }

    cloudc_debug("Exit insertSessionInfo");
    return;
}




void notificationCleanup()
{
    std::cout << "cleanup() - start" << std::endl;
    if (conService) {
        std::cout << "cleanup() - conService" << std::endl;
        conService->shutdown();
        conService = NULL;
    }   
    if (Receiver) {
        std::cout << "cleanup() - Receiver" << std::endl;
        delete Receiver;
        Receiver = NULL;
    }
    std::cout << "cleanup() - end" << std::endl;
}
/* notification add by gaojing end */

/* Print out the fields found in the AboutData. Only fields with known signatures are printed out. 
 * All others will be treated as an unknown field.
 */
void printAboutData(AboutData& aboutData, const char* language)
{
    size_t count = aboutData.GetFields();

    const char** fields = new const char*[count];
    aboutData.GetFields(fields, count);

    for (size_t i = 0; i < count; ++i) {
        std::cout << "\tKey: " << fields[i];

        MsgArg* tmp;
        aboutData.GetField(fields[i], tmp, language);
        std::cout << "\t";
        if (tmp->Signature() == "s") {
            const char* tmp_s;
            tmp->Get("s", &tmp_s);
            std::cout << tmp_s;
        } else if (tmp->Signature() == "as") {
            size_t las;
            MsgArg* as_arg;
            tmp->Get("as", &las, &as_arg);
            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                as_arg[j].Get("s", &tmp_s);
                std::cout << tmp_s << " ";
            }
        } else if (tmp->Signature() == "ay") {
            size_t lay;
            uint8_t* pay;
            tmp->Get("ay", &lay, &pay);
            for (size_t j = 0; j < lay; ++j) {
                std::cout << std::hex << static_cast<int>(pay[j]) << " ";
            }
        } else {
            std::cout << "User Defined Value\tSignature: " << tmp->Signature().c_str();
        }
        std::cout << std::endl;
    }
    delete [] fields;
    std::cout << std::endl;
}

void printAllAboutData(AboutProxy& aboutProxy)
{
    MsgArg aArg;
    QStatus status = aboutProxy.GetAboutData(NULL, aArg);
    if (status == ER_OK) {
        std::cout << "*********************************************************************************" << std::endl;
        std::cout << "GetAboutData: (Default Language)" << std::endl;
        AboutData aboutData(aArg);
        printAboutData(aboutData, NULL);
        size_t lang_num;
        lang_num = aboutData.GetSupportedLanguages();
        std::cout << "Number of supported languages: " << lang_num << std::endl;
        // If the lang_num == 1 we only have a default language
        if (lang_num > 1) {
            const char** langs = new const char*[lang_num];
            aboutData.GetSupportedLanguages(langs, lang_num);
            char* defaultLanguage;
            aboutData.GetDefaultLanguage(&defaultLanguage);
            // print out the AboutData for every language but the
            // default it has already been printed.
            for (size_t i = 0; i < lang_num; ++i) {
                std::cout << "language=" << i << " " << langs[i] << std::endl;
                if (strcmp(defaultLanguage, langs[i]) != 0) {
                    std::cout << "Calling GetAboutData: language=" << langs[i] << std::endl;
                    status = aboutProxy.GetAboutData(langs[i], aArg);
                    if (ER_OK == status) {
                        aboutData.CreatefromMsgArg(aArg, langs[i]);
                        std::cout <<  "GetAboutData: (" << langs[i] << ")" << std::endl;
                        printAboutData(aboutData, langs[i]);
                    } else {
                        std::cout <<  "GetAboutData failed " << QCC_StatusText(status)  << std::endl;
                    }
                }
            }
            delete [] langs;
        }
        std::cout << "*********************************************************************************" << std::endl;
    }
}

void interruptibleDelay(int seconds) {
    for (int i = 0; !s_interrupt && i < seconds; i++) {
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000 * 1000);
#endif
    }
}

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
    {
        //std::cout<<"GaoJing Enter Announced func"<<std::endl;
        cloudc_debug("Enter");
        std::cout<<"GaoJing busname = "<<busName << ", version = " << version << ", port = " << port <<""<<std::endl;

        /* need to add func: send online info to server 
         * if enter this switch, it means this is the device 1st online
         * so just find deviceId by objectPath and interfaceName in objectDescriptionArg
         * then send it to server 
         * */
        char thisInterfaceName[MAX_INTERFACE_LEN] = {0};
        char thisObjectPath[MAX_OBJECTPATH_LEN] = {0};

        CheckIfConfigInterfaceOnline(objectDescriptionArg, aboutDataArg, thisInterfaceName, thisObjectPath, busName, port);
        
        //strcpy(g_busName, busName);
        //g_port = port;
#if 0
        if(remoteConfigFlag == 1)
        {
            busAttachment->EnableConcurrentCallbacks();
#if 0
            // when pinging a remote bus wait a max of 5 seconds

            #define PING_WAIT_TIME    5000
            QStatus status = busAttachment->Ping(busName, 5000);
            if( ER_OK == status) 
            {
                std::cout << "GaoJing ping succeed" << std::endl;
            }
            else
            {
                std::cout << "GaoJing ping fail" << std::endl;
            }
#endif

            SessionId sessionId;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
                    SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            status = busAttachment->JoinSession(busName, port, NULL, sessionId, opts);

            if(status == ER_OK)
            {
                std::cout << "JoinSession Success, sessionId = "<<sessionId<<"" << std::endl;
                remoteConfigFlag = 0;
                std::cout << "objectPath = " << objectPath << std::endl;
                std::cout << "interfaceName = " << interfaceName << std::endl;
                UpdateConfig(busName, sessionId, interfaceName, objectPath, getMsgType, getDevData);
            }
            else
            {
                std::cout << "WARNING - JoinSession failed: " << QCC_StatusText(status) << std::endl;
            }
        }
        else
        {
            std::cout << "GaoJing This announce is no use" <<std::endl;
        }
#endif

        //std::cout<<"GaoJing Exit Announced func"<<std::endl;
        cloudc_debug("Exit");
    }
};

static MyAboutListener* aboutListener = 0;
void WaitForSigInt(void) {
    std::cout<<"GaoJing Enter wait sigInt func "<<std::endl;
    while (s_interrupt == false && s_stopped == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
    std::cout<<"GaoJing Exit wait sigInt func "<<std::endl;
}

int CreateBusAttachment(void) 
{
    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

    std::cout << "*********************************************************************************" << std::endl;
    std::cout << "PLEASE NOTE THAT AS OF NOW THIS PROGRAM DOES NOT SUPPORT INTERACTION WITH THE ALLJOYN THIN CLIENT BASED CONFIGSAMPLE. SO PLEASE USE THIS PROGRAM ONLY WITH ALLJOYN STANDARD CLIENT BASED CONFIGSERVICESAMPLE" << std::endl;
    std::cout << "*********************************************************************************" << std::endl;

    //Enable this line to see logs from config service:
    //QCC_SetDebugLevel(services::logModules::CONFIG_MODULE_LOG_NAME, services::logModules::ALL_LOG_LEVELS);

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */

    signal(SIGINT, SigIntHandler);

    //set Daemon password only for bundled app
#ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
#endif

    busAttachment = new BusAttachment("CloudcConfig", true);

    status = busAttachment->Start();
    if (status == ER_OK) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "ERROR - Unable to start BusAttachment. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    status = busAttachment->Connect();
    if (ER_OK == status) {
        std::cout << "Daemon Connect succeeded." << std::endl;
    } else {
        std::cout << "ERROR - Failed to connect daemon. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }
    return 0;
}

int CheckIfConfigInterfaceOnline(ajn::MsgArg objectDescriptionArg, ajn::MsgArg aboutDataArg, char *interfaceName, char *objectPath, const char *busName, SessionPort port)
{
    QStatus status = ER_OK;
    char *deviceObjectPath = NULL;
    char *deviceInterfaceName = NULL;
    MsgArg* tempArgs;
    size_t objectNum;
    status = objectDescriptionArg.Get("a(oas)", &objectNum, &tempArgs);
    std::cout << "GaoJing objectNum = " << objectNum << std::endl;

    if(status != ER_OK)
    {
        return -1;
    }

    for(size_t i = 0; i < objectNum; i++)
    {
        char* objectDescriptionPath;
        MsgArg* interfaceEnteries;
        size_t interfaceNum;

        status = tempArgs[i].Get("(oas)", &objectDescriptionPath, &interfaceNum, &interfaceEnteries);
        std::cout << "GaoJing objectDescriptionPath " << i << ": " << objectDescriptionPath << ", its interfaceNum = " << interfaceNum << std::endl;

        if(strstr(objectDescriptionPath, "/Config") == NULL)
        {
            std::cout << "Not found ConfigInterface on this objectPath" << std::endl;
            continue;
        }

        memset(objectPath, 0, MAX_OBJECTPATH_LEN);
        deviceObjectPath = objectDescriptionPath;
        strncpy(objectPath, deviceObjectPath, MAX_OBJECTPATH_LEN - 1);
        printf("online 0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ objectPath = %s\n", objectPath);

        if(status != ER_OK)
        {
            return -1;
        }

        for(size_t i = 0; i < interfaceNum; i ++)
        {
            char* interfaceOfAnnounce;
            status = interfaceEnteries[i].Get("s", &interfaceOfAnnounce);
            std::cout << "GaoJing InterfaceOfAnnounce " << i << ": " << interfaceOfAnnounce << std::endl;

            if(status != ER_OK)
            {
                return -1;
            }

            if(strstr(interfaceOfAnnounce, "Config") == NULL)
            {
                continue;
            }

            memset(interfaceName, 0, MAX_INTERFACE_LEN);
            deviceInterfaceName = interfaceOfAnnounce;
            strncpy(interfaceName, deviceInterfaceName, MAX_INTERFACE_LEN - 1);
            insertSessionInfo(interfaceName, busName, port);

            std::set<qcc::String>::iterator searchIterator = handledAnnouncements.find(qcc::String(interfaceName));
            if(searchIterator == handledAnnouncements.end())
            {
                printf("online 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ interfaceName = %s\n", interfaceName);
                char devData[MAX_DEVDATA_LEN] = {0};

                handledAnnouncements.insert(interfaceName);
#if 0
                ParseGetAboutDataArg(aboutDataArg, devData);
#endif
                /* not get devData from aboutData, just get it from the database */
                getDevDataByObj(interfaceName, devData);
                cloudc_send_online_buf(devData); //need to add device_id as arg here

                /* need to update devData in alljoynxml.conf start */
                /* need to update devData in alljoynxml.conf end */
            }
        }

        std::cout << "deviceObjectPath = " << deviceObjectPath << ", deviceInterfaceName = " << deviceInterfaceName << std::endl;
        /* Need to add a func to check whether this configInterface is online or not.
         * if already online on the dataLib, do noting
         * if not online on the dataLib, (and this is its first time to online,) then need to send this info to FEIXUN server.
         * */
    }
    return 0;
}

int ParseGetAboutDataArg(ajn::MsgArg aboutDataArg, char *devData)
{
    std::cout << "GaoJing Enter ParseGetAboutDataArg func" << std::endl;
    QStatus status = ER_OK;
    AboutData aboutData;
    MsgArg* tempControlArg2;
    size_t languageTagNumElements;

    status = aboutDataArg.Get("a{sv}", &languageTagNumElements, &tempControlArg2);
    if (status != ER_OK) 
    {
        std::cout << "Exit ParseGetAboutDataArg func" << std::endl;
        return -1;
    }

    for (size_t i = 0; i < languageTagNumElements; i++) 
    {
        char* tempKey;
        MsgArg* tempValue;
        status = tempControlArg2[i].Get("{sv}", &tempKey, &tempValue);
        if (status != ER_OK) {
            std::cout << "Exit ParseGetAboutDataArg func" << std::endl;
            return -1;
        }

        /* add by gaojing for DevData*/
        if(strcmp(tempKey, "DevData") == 0)
        {
            std::cout << "tempKey = " << tempKey << std::endl;
            std::cout << "tempValue = " << tempValue << std::endl;

            char* value = NULL;
            status = tempValue->Get("s", &value);
            if (status == ER_OK) {
                printf("value = %s\n", value);
            }
            else
            {
                printf("cannot get the value\n");
            }
            strncpy(devData, value, MAX_DEVDATA_LEN - 1);
        }
        /* add by gaojing for DevData*/
    }

    std::cout << "GaoJing Exit ParseGetAboutDataArg func" << std::endl;
    return 0;
}

void HandleUpdateConfigMethod(const char *interfaces)
{
    //std::cout<<"GaoJing Enter HandleUpdateConfigMethod func"<<std::endl;
    cloudc_debug("Enter");
    QStatus status = ER_OK;
    busAttachment->CancelWhoImplements(interfaces);
    status = busAttachment->WhoImplements(interfaces);
    if (ER_OK == status) {
        std::cout << "WhoImplements called." << std::endl;
    } else {
        std::cout << "ERROR - WhoImplements call FAILED with status " << QCC_StatusText(status) << std::endl;
    }

    //std::cout<<"GaoJing Exit HandleUpdateConfigMethod func"<<std::endl;
    cloudc_debug("Exit");
    return ;
}

void UpdateConfig(qcc::String const& busName, unsigned int id, const char *interfaceName, const char *objectPath, int msgType, char *devData)
{
    MsgType msgTypeOption;
    QStatus status = ER_OK;

    //std::cout<<"GaoJing Enter UpdateConfig func"<<std::endl;
    cloudc_debug("Enter");
    AboutProxy aboutProxy(*busAttachment, busName.c_str(), id);

    MsgArg objArg;
    aboutProxy.GetObjectDescription(objArg);
    std::cout << "AboutProxy.GetObjectDescriptions:\n" << objArg.ToString().c_str() << "\n\n" << std::endl;

    AboutObjectDescription objectDescription;
    objectDescription.CreateFromMsgArg(objArg);

    bool isConfigInterface = false;
    if (!s_interrupt) {
        isConfigInterface = objectDescription.HasInterface(objectPath, interfaceName);
        if (isConfigInterface) {
            std::cout << "The given interface '" << interfaceName << "' is found in a given path ' " << objectPath << std::endl;
        } else {
            std::cout << "WARNING - The given interface '" << interfaceName << "' is not found in a given path ' " << objectPath << std::endl;
        }
        //printAllAboutData(aboutProxy);
    }

    services::NewConfigClient* newConfigClient = NULL;
    if (!s_interrupt && isConfigInterface) {
        newConfigClient = new services::NewConfigClient(interfaceName, *busAttachment);
        if (!s_interrupt && newConfigClient) {

            switch(msgType)
            {
                case Get:
                    std::cout << "Get" << std::endl;
                    std::cout << "not used now, so no need to handle ~~" << std::endl;
                    break;

                case Set:
                    //std::cout << "set" << std::endl;
                    cloudc_debug("Set Operation");
                    if (!s_interrupt) 
                    {
                        if(NULL != devData)
                        {
                            if (!s_interrupt) {
                                std::cout << "\nGoing to call to UpdateConfigurations: key=DevData value=" << devData << std::endl;
                                std::cout << "-------------------------------------------------------------------" << std::endl;
                                cloudc_debug("New updateConfigurations");
                                services::NewConfigClient::Configurations updateConfigurations;
                                cloudc_debug("DevDate string insert");
                                updateConfigurations.insert(
                                        std::pair<qcc::String, ajn::MsgArg>("DevData", MsgArg("s", devData)));
                                cloudc_debug("call UpdateConfigurations"); 
                                if ((status = newConfigClient->UpdateConfigurations(busName.c_str(), 
                                                NULL, updateConfigurations, objectPath, interfaceName, id)) == ER_OK) {
                                    std::cout << "UpdateConfigurations DevData succeeded" << std::endl;
                                    ret = 0;
                                } else {
                                    std::cout << "WARNING - Call to UpdateConfigurations DevData failed: " << QCC_StatusText(status) << std::endl;
                                    ret = -1;
                                }
                            }
                        }
                    }
                    cloudc_debug("Set Operation Succ");
                    break;

                case Notify:
                    std::cout << "Notify" << std::endl;
                    break;

                default:
                    std::cout << "Unknown msgType" << std::endl;
                    break;
            }

        } //if (newConfigClient)
    } //if (isConfigInterface)


    if (newConfigClient) {
        delete newConfigClient;
        newConfigClient = NULL;
    }

    //s_stopped = true;
    //callbackFinish = 1;
    cloudc_debug("Exit UpdateConfig");
}

int InteresteForAllConfigInterfaces(const char **interfaces)
{
    std::cout << "GaoJing Enter interest func" << std::endl;
    QStatus status = ER_OK;

    srpKeyXListener = new SrpKeyXListener();
    srpKeyXListener->setPassCode(INITIAL_PASSCODE);

    status = busAttachment->EnablePeerSecurity("ALLJOYN_SRP_KEYX ALLJOYN_PIN_KEYX ALLJOYN_ECDHE_PSK", srpKeyXListener,
            "/.alljoyn_keystore/central.ks", true);

    if (ER_OK == status) {
        std::cout << "ER_BUS_SECURITY__ENABLED" << std::endl;
    } else {
        std::cout << "ERROR - ER_BUS_SECURITY_NOT_ENABLED " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    aboutListener = new MyAboutListener();
    busAttachment->RegisterAboutListener(*aboutListener);

    status = busAttachment->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    if (ER_OK == status) {
        std::cout << "WhoImplements called." << std::endl;
    } else {
        std::cout << "ERROR - WhoImplements call FAILED with status " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    std::cout << "GaoJing Exit interest func" << std::endl;

    return 0;
}

/* notification add by gaojing start 2016.06.28 */
int NotificationInit()
{
    std::cout << "Begin Notification Consumer Application." << std::endl;
    QStatus status = ER_OK;
    std::string listOfApps = "";  //ConfigServiceApp
    /* Set a list of app names (separated by ';') you would like to receive notifications from.
     * Empty list means all app names.
     */

    /* Initialize Service object and send it Notification Receiver object
    */
    conService = NotificationService::getInstance();
    Receiver = new NotificationReceiverTestImpl(false);

    /* Set the list of applications this receiver should receive notifications from
    */
    Receiver->setApplications(listOfApps.c_str());

    status = conService->initReceive(busAttachment, Receiver);
    if (status != ER_OK) {
        std::cout << "Could not initialize receiver." << std::endl;
        conService->shutdown();
        conService = NULL;
        delete Receiver;
        Receiver = NULL;
        return 1;
    }

    std::cout << "Waiting for notifications." << std::endl;
    return 0;
}
/* notification add by gaojing end 2016.06.28 */

void StartAlljoynService()
{
    std::cout << "GaoJing Enter StartAlljoynService func" << std::endl;
    CreateBusAttachment();
    const char* interfaces[] = {"*.*.*.Config"};
    InteresteForAllConfigInterfaces(interfaces);
    //NotificationInit();
    std::cout << "GaoJing Exit StartAlljoynService func" << std::endl;
}

void StopAlljoynService()
{
    std::cout << "GaoJing Enter StopAlljoynService func" << std::endl;
    const char* interfaces[] = {"*.*.*.Config"};
    /* notification add by gaojing start */
    //notificationCleanup(); 
    /* notification add by gaojing end */
    busAttachment->CancelWhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    busAttachment->UnregisterAboutListener(*aboutListener);
    busAttachment->EnablePeerSecurity(NULL, NULL, NULL, true);

    delete srpKeyXListener;
    delete aboutListener;
    srpKeyXListener = 0;
    aboutListener = 0;

    busAttachment->Stop();
    delete busAttachment;
    busAttachment = 0;
    std::cout << "GaoJing Exit StopAlljoynService func" << std::endl;
}


int configClientMain(char *interfaces, char *matchObjectPath, int msgType, char *devData)
{
    sessionInfo *node = NULL;

    //std::cout << "GaoJing Enter configClientMain func" << std::endl;
    cloudc_debug("Enter");

    if(busAttachment == 0)
    {
        std::cout << "GaoJing Exit configClientMain func" << std::endl;
        return -1;
    }
    std::cout <<"busAttachment = " << busAttachment << ", interfaces = " << interfaces << ", matchObjectPath = " << matchObjectPath << "" <<std::endl;
    //remoteConfigFlag = 1;
    interfaceName = interfaces;
    objectPath = matchObjectPath;
    getMsgType = msgType;
    getDevData = devData;

    //HandleUpdateConfigMethod(interfaces);
    node = findSessionInfo(interfaces);
  /*  node = sessionInfoList;
    char tmp[512] = "Philips.led.001788010231d62c0b.Config";
    while(node)
    {
        if (strcmp(tmp, node->interfaceName) == 0)
        {
            cloudc_debug("find session info[%s]", interfaceName);
            break;
        }

        node = node->next;
    }
    */

    if (node == NULL)
    {
        cloudc_error("interface [%s] don't exist");
        return -1;
    }

    UpdateConfig(node->busName, node->sessionId, interfaceName, objectPath, getMsgType, getDevData);
#if 0
    node = (sessionInfo *)malloc(sizeof(sessionInfo));
    if (node != NULL)
    {
        strncpy(node->interfaceName, interfaceName, sizeof(node->interfaceName));
        strncpy(node->busName, g_busName, sizeof(node->busName));
        node->port = g_port;
    }
    else
    {
        cloudc_debug("malloc session node failed");
        return -1;
    }

    QStatus status = ER_OK;
    SessionId sessionId;
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false,
            SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = busAttachment->JoinSession(node->busName, node->port, NULL, sessionId, opts);

    if(status == ER_OK)
    {
        std::cout << "JoinSession Success, sessionId = "<<sessionId<<"" << std::endl;
        std::cout << "objectPath = " << objectPath << std::endl;
        std::cout << "interfaceName = " << interfaceName << std::endl;
    }
    else
    {
        std::cout << "WARNING - JoinSession failed: " << QCC_StatusText(status) << std::endl;
    }

    //UpdateConfig(node->busName, sessionId, interfaceName, objectPath, getMsgType, getDevData);

    status = busAttachment->LeaveSession(sessionId);
    std::cout << "Leaving session id = " << sessionId << " with " << node->busName << " status: " << QCC_StatusText(status) << std::endl;
#endif    
#if 0
    HandleUpdateConfigMethod(interfaces);

    while(callbackFinish != 1)
    {
        usleep(100*1000);
    }
    s_interrupt = false;
    s_stopped = false;
    callbackFinish = 0;
#endif
    //std::cout << "GaoJing Exit configClientMain func" << std::endl;
    cloudc_debug("Exit");
    return ret;

} /* main() */

void cleanup()
{
    // Clean up
    if (prodService) {
        prodService->shutdown();
        prodService = NULL;
    }
    if (busAttachment && busListener) {
        CommonSampleUtil::aboutServiceDestroy(busAttachment, busListener);
    }
    if (busListener) {
        delete busListener;
        busListener = NULL;
    }
    if (aboutData) {
        delete aboutData;
        aboutData = NULL;
    }
    if (aboutObj) {
        delete aboutObj;
        aboutObj = NULL;
    }
    if (busAttachment) {
        delete busAttachment;
        busAttachment = NULL;
    }
    std::cout << "Goodbye!" << std::endl;
}

void signal_callback_handler(int32_t signum)
{
    std::cout << "got signal_callback_handler" << std::endl;
    s_interrupt = true;
}

int NotificationSendMain(char *msg_type, int serial, char *deviceid, char *userid, char *ImpPath)
{
    cloudc_debug("Enter!");
    
//    NotificationService* prodService = 0;
//    NotificationSender* Sender = 0;
//    BusAttachment* bus = 0;
 //   AboutData* aboutData = NULL;
//    AboutObj* aboutObj = NULL;
//    CommonBusListener*  busListener = 0;
    
    char js_buf[SEND_MAX_BUF_LEN] = {0};

    // Allow CTRL+C to end application
    signal(SIGINT, signal_callback_handler);

    // Initialize Service object and Sender Object
    prodService = NotificationService::getInstance();

//set Daemon password only for bundled app
#ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
#endif
    QStatus status;

    QCC_SetDebugLevel(logModules::NOTIFICATION_MODULE_LOG_NAME, logModules::ALL_LOG_LEVELS);

    busAttachment = CommonSampleUtil::prepareBusAttachment();
    if (busAttachment == NULL) {
        std::cout << "Could not initialize BusAttachment." << std::endl;
        return 1;
    }

   qcc::String deviceId;
   GuidUtil::GetInstance()->GetDeviceIdString(&deviceId);
   qcc::String appid;
   GuidUtil::GetInstance()->GenerateGUID(&appid);

    aboutData = new AboutData("en");
    DeviceNamesType deviceNames;
    deviceNames.insert(std::pair<qcc::String, qcc::String>("en", "fxagent"));
    status = CommonSampleUtil::fillAboutData(aboutData, appid, APP_NAME, deviceId, deviceNames);
    if (status != ER_OK) {
        std::cout << "Could not fill About Data." << std::endl;
        cleanup();
        return 1;
    }

    busListener = new CommonBusListener();

    aboutObj = new AboutObj(*busAttachment, BusObject::ANNOUNCED);
    status = CommonSampleUtil::prepareAboutService(busAttachment, aboutData, aboutObj, busListener, SERVICE_PORT);
    if (status != ER_OK) {
        std::cout << "Could not set up the AboutService." << std::endl;
        cleanup();
        return 1;
    }

    Sender = prodService->initSend(busAttachment, aboutData);
    if (!Sender) {
        std::cout << "Could not initialize Sender - exiting application" << std::endl;
        cleanup();
        return 1;
    }

    status = CommonSampleUtil::aboutServiceAnnounce();
    if (status != ER_OK) {
        std::cout << "Could not announce." << std::endl;
        cleanup();
        return 1;
    }
    // Prepare message type
    NotificationMessageType messageType = INFO;
    //add cloudc_build_notification_js_bu
    int ret1 = -1;
    ret1 = cloudc_build_notification_js_buf(msg_type, serial, deviceid, userid, ImpPath, js_buf);
    qcc::String TEXT1 = js_buf;
    // Prepare text object, set language and text, add notification to vector
    NotificationText textToSend1(LANG1, TEXT1);
    //NotificationText textToSend2(LANG2, TEXT2);

    std::vector<NotificationText> vecMessages;
    vecMessages.push_back(textToSend1);
    //vecMessages.push_back(textToSend2);

/*    // Add variable parameters
    std::map<qcc::String, qcc::String> customAttributes;
    customAttributes[KEY1] = VAL1;
    customAttributes[KEY2] = VAL2;

    //Prepare Rich Notification Content
    RichAudioUrl audio1(LANG1, URL1);
    RichAudioUrl audio2(LANG2, URL2);

    std::vector<RichAudioUrl> richAudioUrl;

    richAudioUrl.push_back(audio1);
    richAudioUrl.push_back(audio2);

    qcc::String richIconUrl = RICH_ICON_URL;
    qcc::String richIconObjectPath = RICH_ICON_OBJECT_PATH;
    qcc::String richAudioObjectPath = RICH_AUDIO_OBJECT_PATH;
    qcc::String controlPanelServiceObjectPath = CONTROL_PANEL_SERVICE_OBJECT_PATH;
*/
    // Send message 
    Notification notification(messageType, vecMessages);
   // notification.setCustomAttributes(customAttributes);
   // notification.setRichIconUrl(richIconUrl.c_str());
   // notification.setRichAudioUrl(richAudioUrl);
   // notification.setRichIconObjectPath(richIconObjectPath.c_str());
   // notification.setRichAudioObjectPath(richAudioObjectPath.c_str());
   // notification.setControlPanelServiceObjectPath(controlPanelServiceObjectPath.c_str());

    status = Sender->send(notification, 7200);
    if (status != ER_OK) {
        std::cout << "Notification was not sent successfully - exiting application" << std::endl;
        cleanup();
        return 1;
    }

    std::cout << "Notification sent! " << std::endl;
    std::cout << "Hit Ctrl+C to exit the application" << std::endl;

    //WaitForSigInt();

    std::cout << "Exiting the application and deleting the busAttachment connection." << std::endl;
    cleanup();

    return 0;
}
