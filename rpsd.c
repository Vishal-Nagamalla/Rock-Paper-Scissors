#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include "rps_utils.h"

#define MAX_WAITING 100
#define MAX_ACTIVE 100

static waiting_player_t waiting_queue[MAX_WAITING];
static int waiting_count = 0;

static char active_names[MAX_ACTIVE][50];
static int active_count = 0;

static pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_thread(void *arg);
void enqueue_player(int sockfd, const char *name);
int dequeue_two_players(waiting_player_t *p1, waiting_player_t *p2);
int is_name_active(const char *name);
void add_active_name(const char *name);
void remove_active_name(const char *name);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("rpsd listening on port %d...\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char host[NI_MAXHOST], serv[NI_MAXSERV];
        if (getnameinfo((struct sockaddr *)&client_addr, addr_len, host, sizeof(host), serv, sizeof(serv),
                        NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            printf("Connection from %s:%s\n", host, serv);
        }

        pthread_t tid;
        int *fd_ptr = malloc(sizeof(int));
        if (!fd_ptr) {
            fprintf(stderr, "Out of memory\n");
            close(client_fd);
            continue;
        }
        *fd_ptr = client_fd;

        if (pthread_create(&tid, NULL, client_thread, fd_ptr) != 0) {
            perror("pthread_create");
            free(fd_ptr);
            close(client_fd);
        } else {
            pthread_detach(tid);
        }
    }

    close(listen_fd);
    return 0;
}

void *client_thread(void *arg)
{
    int sockfd = *((int *)arg);
    free(arg);

    char name_out[50];
    if (read_play_message(sockfd, name_out, sizeof(name_out)) < 0) {
        close(sockfd);
        return NULL;
    }
    if (write_wait_message(sockfd) < 0) {
        close(sockfd);
        return NULL;
    }

    enqueue_player(sockfd, name_out);
    return NULL;
}

int is_name_active(const char *name) {
    for (int i = 0; i < active_count; ++i) {
        if (strcmp(active_names[i], name) == 0) return 1;
    }
    return 0;
}

void add_active_name(const char *name) {
    if (active_count < MAX_ACTIVE) {
        strncpy(active_names[active_count++], name, 49);
    }
}

void remove_active_name(const char *name) {
    for (int i = 0; i < active_count; ++i) {
        if (strcmp(active_names[i], name) == 0) {
            for (int j = i; j < active_count - 1; ++j) {
                strcpy(active_names[j], active_names[j + 1]);
            }
            active_count--;
            return;
        }
    }
}

void enqueue_player(int sockfd, const char *name)
{
    pthread_mutex_lock(&waiting_mutex);

    if (is_name_active(name)) {
        write(sockfd, "R|L|Logged in||", 15);
        close(sockfd);
        pthread_mutex_unlock(&waiting_mutex);
        return;
    }

    if (waiting_count >= MAX_WAITING) {
        printf("Queue full. Closing client.\n");
        close(sockfd);
        pthread_mutex_unlock(&waiting_mutex);
        return;
    }

    strncpy(waiting_queue[waiting_count].name, name, sizeof(waiting_queue[waiting_count].name) - 1);
    waiting_queue[waiting_count].name[sizeof(waiting_queue[waiting_count].name) - 1] = '\0';
    waiting_queue[waiting_count].sockfd = sockfd;
    waiting_count++;

    add_active_name(name);

    if (waiting_count >= 2) {
        waiting_player_t p1, p2;
        if (dequeue_two_players(&p1, &p2)) {
            match_args_t *margs = malloc(sizeof(match_args_t));
            if (!margs) {
                fprintf(stderr, "Out of memory for match args\n");
                close(p1.sockfd);
                close(p2.sockfd);
            } else {
                margs->player1 = p1;
                margs->player2 = p2;
                pthread_t match_tid;
                if (pthread_create(&match_tid, NULL, match_thread, margs) != 0) {
                    perror("pthread_create match_thread");
                    free(margs);
                    close(p1.sockfd);
                    close(p2.sockfd);
                    remove_active_name(p1.name);
                    remove_active_name(p2.name);
                } else {
                    pthread_detach(match_tid);
                }
            }
        }
    }

    pthread_mutex_unlock(&waiting_mutex);
}

int dequeue_two_players(waiting_player_t *p1, waiting_player_t *p2)
{
    if (waiting_count < 2) {
        return 0;
    }
    *p1 = waiting_queue[0];
    *p2 = waiting_queue[1];

    waiting_count -= 2;
    for (int i = 0; i < waiting_count; i++) {
        waiting_queue[i] = waiting_queue[i + 2];
    }
    return 1;
}