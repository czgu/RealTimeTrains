#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#include <task_type.h>

#define MAXNAMESIZE 20
#define DICTIONARYSIZE 100

#define NAMESERVER_TID 65537 //TODO: find a better way to get tid

typedef struct NSbinding {
    unsigned int tid;
    char name[MAXNAMESIZE];
} NSbinding;

typedef enum {
    REGISTERAS = 0,
    WHOIS
} NSOP;

typedef struct NSmsg {
    NSOP opcode;
    NSbinding binding;
    int err;
} NSmsg;

typedef struct Dictionary {
    NSbinding data[DICTIONARYSIZE];
    unsigned int size;
} Dictionary;

void dictionary_init(Dictionary*);
int dictionary_add(Dictionary*, NSbinding);
int dictionary_find(Dictionary*, char*);
int dictionary_update(Dictionary* dic, NSbinding bnd, int index);

// Server task
void nameserver_task();

#endif
