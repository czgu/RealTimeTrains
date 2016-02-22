#include <train_logic_task.h>
#include <terminal_mvc_server.h>
#include <train.h>
#include <syscall.h>

void train_command_task() {
    int i;

    TERMmsg command;

    int terminal_server_tid = WhoIs("term controller");
    char requestOP = TRAIN_CMD_READY;

    Train trains[81];
    for (i = 0; i < 81; i++) {
        train_init(trains + i, i);
    }

    // init switch
    for (i = 1; i <= NUM_TRAIN_SWITCH; i++) {
        train_cmd(SWITCH_DIR_C, i);
    }
    train_soloff();

    for (;;) {
        int sz = Send(terminal_server_tid, &requestOP, sizeof(char), &command, sizeof(TERMmsg));
        if (sz >= sizeof(char)) {
            switch(command.opcode) {
                case CMD_TR:
                    train_setspeed(trains + command.param[0], command.param[1]);
                    break; 
                case CMD_RV: {
                    command.param[1] = trains[(int)command.param[0]].speed;

                    int cid = Create(10, train_reverse_task);
                    Send(cid, &command, sizeof(TERMmsg), 0, 0);
                    break;
                }
                case CMD_SW: {
                    train_cmd(command.param[1], command.param[0]);
                    Create(10, train_switch_task);
                    break;
                }
            }
        }
    }
}

void train_reverse_task() {
    int sender;
    TERMmsg command;

    Receive(&sender, &command, sizeof(TERMmsg));
    Reply(sender, 0, 0);

    if (command.opcode == CMD_RV) {
        train_cmd(0, command.param[0]);
        Delay(300);
        train_cmd(15, command.param[0]);
        train_cmd(command.param[1], command.param[0]);
    }

}

void train_switch_task() {
    // turn off solenoid
    Delay(10);
    train_soloff();
}
