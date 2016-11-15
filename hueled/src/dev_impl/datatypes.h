#ifndef __DEV_DATATYPE_H__
#define __DEV_DATATYOE_H__

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

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
    char *manufacture;
    char *manufactureSN;
} client_t;

typedef slist_t clients_t;
#endif
