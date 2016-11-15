include $(TOPDIR)/rules.mk

TARGET_CFLAGS = -g
DLIB  = -lblobmsg_json -lm -lpthread \
		-lalljoyn -lalljoyn_about -lalljoyn_config -lalljoyn_controlpanel -lalljoyn_services_common -lalljoyn_notification -lsqlite3
#		-lmxml
DLIB_FLAGS = 
OBJ = cJSON.o parser.o sqlite3.o log.o \
	  OptParser.o ConfigServiceListenerImpl.o \
	  AboutDataStore.o AsyncSessionJoiner.o \
	  AnnounceHandlerImpl.o CommonSampleUtil.o \
	  SrpKeyXListener.o AboutObjApi.o \
	  CommonBusListener.o SessionListenerImpl.o \
	  ConfigServiceMain.o 

#TARGET_CPPFLAGS += -DTCP_PROXY_CTL_INTF
#OBJ += pCtlIntf.o devImpl.o

#TARGET_CFLAGS += -DIP_DEV_CTL_SERVER
#TARGET_CPPFLAGS += -DIP_DEV_CTL_SERVER
#OBJ += devMgmt.o msgHandle.o \
	   msg.o clients.o slist.o
#TARGET_CFLAGS += -I ./dev_impl



all_programs = hueled
all:$(all_programs)

.PHONY:all

$(all_programs): $(OBJ)
	$(TARGET_CXX) $(TARGET_LDFLAGS) $(TARGET_CPPFLAGS) $(DLIB_FLAGS) $(DLIB) $^ -o $@ 

%.o:%.c
	$(TARGET_CC) $(TARGET_CFLAGS) -c $<  

#%.o:dev_impl/%.c
#	$(TARGET_CC) $(TARGET_CFLAGS) -c $<  
#	$(TARGET_CXX) $(TARGET_CPPFLAGS) -I. -I./dev_impl -O2 -g -c $<  

%.o:%.cc
	$(TARGET_CXX) $(TARGET_CPPFLAGS) -I./samples_common -DQCC_OS_GROUP_POSIX -DQCC_OS_LINUX -O2 -g -c $<  

%.o:samples_common/%.cc
	$(TARGET_CXX) $(TARGET_CPPFLAGS) -I. -I./samples_common -DQCC_OS_GROUP_POSIX -DQCC_OS_LINUX -DNEED_DATA_STORE -O2 -g -c $<  

#%.o:common/%.cc
#	$(TARGET_CXX) $(TARGET_CPPFLAGS) -I./common -DQCC_OS_GROUP_POSIX -DQCC_OS_LINUX -O2 -g -c $<
clean:
	rm -rf $(OBJ_PATH) $(all_programs)
