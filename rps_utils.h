#ifndef RPS_UTILS_H
#define RPS_UTILS_H

#include <arpa/inet.h>

typedef struct {
    int sockfd;
    char name[50];
} waiting_player_t;

typedef struct {
    waiting_player_t player1;
    waiting_player_t player2;
} match_args_t;

void *match_thread(void *arg);

int read_play_message(int sockfd, char *name_out, size_t name_size);

int write_wait_message(int sockfd);

char determine_result(const char *move1, const char *move2);

void send_forfeit(const waiting_player_t *winner, const waiting_player_t *forfeiter);

#endif