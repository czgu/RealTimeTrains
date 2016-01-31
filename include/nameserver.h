#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

#define NAMESERVER_SIZE 40
typedef struct NameServer {
    char name[NAMESERVER_SIZE];
    unsigned int pid;
} NameServer;

void nameserver_init(NameServer* ns);

#endif
