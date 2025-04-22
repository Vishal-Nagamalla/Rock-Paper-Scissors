 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 
 static int read_message(int sockfd, char *buf, size_t bufsize);
 
 int main(int argc, char *argv[])
 {
     if (argc != 4) {
         fprintf(stderr, "Usage: %s <serverIP> <serverPort> <playerName>\n", argv[0]);
         return 1;
     }
 
     char *serverIP = argv[1];
     int port = atoi(argv[2]);
     char *playerName = argv[3];
 
     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
         perror("socket");
         return 1;
     }
 
     struct sockaddr_in serv_addr;
     memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port = htons(port);
 
     if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0) {
         perror("inet_pton");
         close(sockfd);
         return 1;
     }
 
     if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("connect");
         close(sockfd);
         return 1;
     }
 
     char playMsg[128];
     snprintf(playMsg, sizeof(playMsg), "P|%s||", playerName);
     write(sockfd, playMsg, strlen(playMsg));
 
     {
         char buf[256];
         if (read_message(sockfd, buf, sizeof(buf)) > 0) {
             printf("Server: [%s]\n", buf);
         } else {
             printf("Server closed?\n");
             close(sockfd);
             return 0;
         }
     }
 
     while (1) {
         char buf[256];
         int res = read_message(sockfd, buf, sizeof(buf));
         if (res <= 0) {
             printf("Server disconnected.\n");
             break;
         }
         printf("Server: [%s]\n", buf);
 
         if (buf[0] == 'B') {
             write(sockfd, "M|ROCK||", 8);
         }
         else if (buf[0] == 'R') {
            static int count = 0;
            if (count < 2) {
                write(sockfd, "C||", strlen("C||"));
            } else {
                write(sockfd, "Q||", strlen("Q||"));
            }
            count++;
        } 
     }
 
     close(sockfd);
     return 0;
 }
 
 static int read_message(int sockfd, char *buf, size_t bufsize)
 {
     memset(buf, 0, bufsize);
     size_t total_read = 0;
 
     while (total_read < bufsize - 1) {
         int n = read(sockfd, buf + total_read, 1);
         if (n <= 0) {
             return -1;
         }
         total_read += n;
         if (total_read >= 2 &&
             buf[total_read - 2] == '|' &&
             buf[total_read - 1] == '|') {
             buf[total_read - 2] = '\0';
             return (int)total_read - 2;
         }
     }
     return -1;
 }