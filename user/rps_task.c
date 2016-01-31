#include "rps_task.h"

#include <bwio.h>
#include <rqueue.h>
#include <string.h>
#include <syscall.h>

void add_player(RPS_Client* player, RQueue* waiting_queue, int *num_playing) {
    int i = *num_playing;
    if (i < 2 && waiting_queue->size >= 1) {
        player[i] = *((RPS_Client*)rq_pop_front(waiting_queue));
        player[i].request.ret = 0;
        Reply(player[i].tid, &player[i].request, sizeof(RPS_Request));
        *num_playing = i + 1;
    }
}

void play_rps(RPS_Client* player) {
    char m0 = player[0].request.param;
    char m1 = player[1].request.param;

    // 0 (lose), 1 (tie), 2 (win)
    int result[2] = {1, 1};

    if (m1 != m0) {
        switch(m0) {
        case 'R':
            result[0] = (m1 == 'S'? 2 : 0);
            break;
        case 'P':
            result[0] = (m1 == 'R'? 2 : 0);
            break;
        case 'S':
            result[0] = (m1 == 'P'? 2 : 0);
            break;
        default:
            break;
        }
        result[1] = (result[0] == 2? 0 : 2);
    }

    int i;
    for (i = 0; i < 2; i++) {
        player[i].request.ret = result[i];
        Reply(player[i].tid, &player[i].request, sizeof(RPS_Request));
    }
}

int get_player_idx(int sender, RPS_Request* request, RPS_Client* player) {
    int player_idx = -1;
    int i;
    for (i = 0; i < 2; i++) {
        if (sender == player[i].tid) {
            player_idx = i;
            break;
        }
    }

    // not actually player
    if (player_idx < 0) {
        request->ret = -1;
        Reply(sender, request, sizeof(RPS_Request));
    }

    return player_idx;
}



void rps_server() {
    // initialize client struct
    int num_playing = 0; int player_moved = 0;
    RPS_Client player[2];
    player[0].tid = player[1].tid = 0;

    RPS_Client waiting_player[9];
    RQueue waiting_queue;
    rq_init(&waiting_queue, waiting_player, 9, sizeof(RPS_Client));

    RegisterAs("RPS Server");
    
    // create clients
    bwprintf(COM2, "(Server) How many players (0-9)?");
    int num_player = a2d(bwgetc(COM2));
    bwprintf(COM2, "\n\r");
    
    if (num_player < 0 || num_player > 9) {
        bwprintf(COM2, "Invalid num of players (%d)\n\r", num_player);
    }
    while(num_player --> 0)
        Create(MED, rps_client);

    for (;;) {
        RPS_Request request;
        int sender;
        Receive(&sender, &request, sizeof(request));
        

        switch(request.opcode) {
        case 'S': {
            RPS_Client client;
            client.request = request;
            client.tid = sender;

            rq_push_back(&waiting_queue, &client);
            add_player(player, &waiting_queue, &num_playing);
            break;
        }
        case 'P': {
            int player_idx = get_player_idx(sender, &request, player);

            if (player_idx < 0)
                break;

            player[player_idx].request = request;
            player_moved++;
            if (player_moved == 2) {
                play_rps(player);
                player_moved = 0;
            }

            break;
        }
        case 'Q': {
            int player_idx = get_player_idx(sender, &request, player);
            if (player_idx < 0)
                break;
            
            num_playing --;

            if (player_idx == 0) {
                player[0] = player[1];
            }
            player[1].tid = 0;
            
            Reply(sender, &request, sizeof(RPS_Request));
            add_player(player, &waiting_queue, &num_playing);

            break;
        }
        default:
            bwprintf(COM2, "invalid request");
            break;
        }
  
    }

}

void rps_client() {
    int server_tid = -1;
    while ((server_tid = WhoIs("RPS Server")) < 0);

    int tid = MyTid();

    int playing = 0;

    RPS_Request request;
    for (;;) {
        if (playing == 0) {
            bwprintf(COM2, "[%d] Do you want to sign up? (y/n)", tid);
            char ans = bwgetc(COM2);
            bwprintf(COM2, "\n\r");

            if (ans == 'y' || ans == 'Y') {
                request.opcode = 'S';
                Send( server_tid, &request, sizeof(request), &request, sizeof(request));
                playing = 1;
                bwprintf(COM2, "[%d] Signed up.\n\r", tid);
            } else {
                bwprintf(COM2, "[%d] Bye bye!\n\r", tid);
                break;
            } 
        } else {
            bwprintf(COM2, "[%d] What move are you going to make? Press other key to quit\n\r(R:rock, P:paper: S:scissor)", tid);
            char ans = bwgetc(COM2);
            bwprintf(COM2, "\n\r");

            if (ans == 'R' || ans == 'P' || ans == 'S') {
                request.opcode = 'P';
                request.param = ans;
                Send( server_tid, &request, sizeof(request), &request, sizeof(request));

                switch(request.ret) {
                case 0:
                    bwprintf(COM2, "[%d] You lost!\n\r", tid);
                    break;
                case 1:
                    bwprintf(COM2, "[%d] You tied!\n\r", tid);
                    break;
                case 2:
                    bwprintf(COM2, "[%d] You won!\n\r", tid);
                    break;
                default:
                    bwprintf(COM2, "[%d] Unknown output: %d\n\r", tid, request.ret);
                    break;
                }

            } else {
                request.opcode = 'Q';
                Send( server_tid, &request, sizeof(request), &request, sizeof(request));

                bwprintf(COM2, "[%d] Quit game\n\r", tid);
                playing = 0;
            }
        }
        

    }
}
