#ifndef __DEV_DATATYPE_H__
#define __DEV_DATATYOE_H__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#define MAX_MANUFACTURE_LEN 16
#define MAX_MANUFACTURESN_LEN 256

typedef enum {
    DEV_MSG_DEV_ONLINE=0,
} DevMsgType;

typedef struct dev_msg_header
{
   DevMsgType  type;
   unsigned int clientId;
   unsigned int seq;
   unsigned int wordData;
   int dataLength;
} DevMsgHeader;


struct slist_element_struct {
    void* data_;
    struct slist_element_struct* next_;
};
typedef struct slist_element_struct slist_element_t;

struct slist_struct {
    void (*delete_element)(void* element);
    slist_element_t* first_;
};
typedef struct slist_struct slist_t;

typedef struct {
    int fd_;
    unsigned int clientId;
    unsigned int seq;
} client_t;

typedef slist_t clients_t;
#endif
