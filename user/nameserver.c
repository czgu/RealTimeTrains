#include <nameserver.h>

void nameserver_init(NameServer* ns) {
    int i;
    for (i = 0; i < NAMESERVER_SIZE; i++) {
        ns->pid = 0;
        ns->name[0] = 0;
    }
}


