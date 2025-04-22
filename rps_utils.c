#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rps_utils.h"

extern void enqueue_player(int sockfd, const char *name);
extern void remove_active_name(const char *name);

static int read_message(int sockfd, char *buf, size_t bufsize);
static int write_message(int sockfd, const char *msg);
int read_move(int sockfd, char *move_out, size_t move_size);
int read_continue_or_quit(int sockfd);

void *match_thread(void *arg)
{
    match_args_t *margs = (match_args_t *)arg;
    waiting_player_t p1 = margs->player1;
    waiting_player_t p2 = margs->player2;
    free(margs);

    printf("Match started between %s and %s\n", p1.name, p2.name);

    int stillPlaying = 1;
    while (stillPlaying) {
        char msg1[128], msg2[128];
        snprintf(msg1, sizeof(msg1), "B|%s||", p2.name);
        snprintf(msg2, sizeof(msg2), "B|%s||", p1.name);
        if (write_message(p1.sockfd, msg1) < 0 || write_message(p2.sockfd, msg2) < 0) {
            send_forfeit(&p1, &p2);
            goto cleanup;
        }

        char move1[16], move2[16];
        if (read_move(p1.sockfd, move1, sizeof(move1)) < 0) {
            send_forfeit(&p2, &p1);
            goto cleanup;
        }
        if (read_move(p2.sockfd, move2, sizeof(move2)) < 0) {
            send_forfeit(&p1, &p2);
            goto cleanup;
        }

        char res1 = determine_result(move1, move2);
        char res2 = (res1 == 'W') ? 'L' : (res1 == 'L') ? 'W' : 'D';

        snprintf(msg1, sizeof(msg1), "R|%c|%s||", res1, move2);
        snprintf(msg2, sizeof(msg2), "R|%c|%s||", res2, move1);
        if (write_message(p1.sockfd, msg1) < 0 || write_message(p2.sockfd, msg2) < 0) {
            goto cleanup;
        }

        int c1 = read_continue_or_quit(p1.sockfd);
        int c2 = read_continue_or_quit(p2.sockfd);

        if (c1 < 0 || c2 < 0) goto cleanup;

        if (c1 == 1 && c2 == 1) {
            continue;
        } else if (c1 == 1) {
            enqueue_player(p1.sockfd, p1.name);
            close(p2.sockfd);
        } else if (c2 == 1) {
            enqueue_player(p2.sockfd, p2.name);
            close(p1.sockfd);
        } else {
            close(p1.sockfd);
            close(p2.sockfd);
        }

        break;  // match ends
    }

cleanup:
    printf("Match ended between %s and %s\n", p1.name, p2.name);
    remove_active_name(p1.name);
    remove_active_name(p2.name);
    return NULL;
}

int read_play_message(int sockfd, char *name_out, size_t name_size)
{
    char buf[128];
    if (read_message(sockfd, buf, sizeof(buf)) <= 0) {
        return -1;
    }
    if (strncmp(buf, "P|", 2) != 0) return -1;
    char tmp[64];
    if (sscanf(buf, "P|%63[^|]", tmp) != 1) return -1;
    strncpy(name_out, tmp, name_size - 1);
    name_out[name_size - 1] = '\0';
    return 0;
}

int write_wait_message(int sockfd)
{
    return write_message(sockfd, "W|1||");
}

int read_move(int sockfd, char *move_out, size_t move_size)
{
    char buf[128];
    if (read_message(sockfd, buf, sizeof(buf)) <= 0) return -1;
    if (strncmp(buf, "M|", 2) != 0) return -1;
    char tmp[16];
    if (sscanf(buf, "M|%15[^|]", tmp) != 1) return -1;
    strncpy(move_out, tmp, move_size - 1);
    move_out[move_size - 1] = '\0';
    return 0;
}

int read_continue_or_quit(int sockfd)
{
    char buf[32];
    if (read_message(sockfd, buf, sizeof(buf)) <= 0) return -1;
    if (strcmp(buf, "C") == 0) return 1;
    if (strcmp(buf, "Q") == 0) return 0;
    return -1;
}

void send_forfeit(const waiting_player_t *winner, const waiting_player_t *forfeiter)
{
    write_message(winner->sockfd, "R|F|||");
}

char determine_result(const char *move1, const char *move2)
{
    if (strcasecmp(move1, move2) == 0) return 'D';
    if ((strcasecmp(move1, "ROCK") == 0 && strcasecmp(move2, "SCISSORS") == 0) ||
        (strcasecmp(move1, "SCISSORS") == 0 && strcasecmp(move2, "PAPER") == 0) ||
        (strcasecmp(move1, "PAPER") == 0 && strcasecmp(move2, "ROCK") == 0))
        return 'W';
    return 'L';
}

static int read_message(int sockfd, char *buf, size_t bufsize)
{
    memset(buf, 0, bufsize);
    size_t total_read = 0;
    while (total_read < bufsize - 1) {
        int n = read(sockfd, buf + total_read, 1);
        if (n <= 0) return -1;
        total_read += n;
        if (total_read >= 2 && buf[total_read - 2] == '|' && buf[total_read - 1] == '|') {
            buf[total_read - 2] = '\0';
            return total_read - 2;
        }
    }
    return -1;
}

static int write_message(int sockfd, const char *msg)
{
    size_t len = strlen(msg);
    return (write(sockfd, msg, len) < 0) ? -1 : 0;
}
