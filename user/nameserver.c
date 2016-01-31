#include <nameserver.h>
#include <string.h>
#include <syscall.h>

void dictionary_init(Dictionary* dic) {
    dic->size = 0;
}

int dictionary_add(Dictionary* dic, NSbinding bnd) {
    if (dic->size >= DICTIONARYSIZE) {
        return -1;
    }
    NSbinding* data = dic->data;
    data[dic->size] = bnd;
    dic->size++;
    return 0;
}

int dictionary_update(Dictionary* dic, NSbinding bnd, int index) {
    if (index < 0 || index >= dic->size) {
        return -1;
    }
    dic->data[index] = bnd;
    return 0;
}

int dictionary_find(Dictionary* dic, char* name) {
    int i;
    for (i = 0; i < dic->size; i++) {
        if (strcmp(dic->data[i].name, name) == 0) {
            //*tid = dic->data[i].tid;
            return i;
        }
    }
    return -1;
}

void nameserver_task() {
    Dictionary dict;
    dictionary_init(&dict);

    for (;;) {
        int sender;
        NSmsg msg;
        int sz = Receive(&sender, &msg, sizeof(NSmsg));
        // TODO: check sz

        switch(msg.opcode) {
            case REGISTERAS: {
                int index = dictionary_find(&dict, msg.binding.name);
                if (index >= 0) {
                    msg.err = dictionary_update(&dict, msg.binding, index);
                } else {
                    msg.err = dictionary_add(&dict, msg.binding);
                }
                break;
            }
            case WHOIS: {
                int index = dictionary_find(&dict, msg.binding.name);
                if (index >= 0) {
                    msg.binding.tid = dict.data[index].tid;
                    msg.err = 0;
                } else {
                    // name not found
                    msg.binding.tid = 0;
                    msg.err = -1;
                }
                break;
            }
            // TODO: add method to exit nameserver_task?
            default:
                // TODO: throw error
                break;
        }
        Reply(sender, (void*) &msg, sizeof(NSmsg));
    }
}
