#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "lib.h"

void usage(char * file) {
    fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
    exit(0);
}

int main(int argc, char * argv[]) {
    int sockfd_tcp, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN], aux[BUFLEN];
    char id_client[IDLEN];
    topic topics[BUFLEN];
    int topic_num = 1;

    fd_set read_fds;
    fd_set tmp_fds;

    int fdmax;

    FD_ZERO( & tmp_fds);
    FD_ZERO( & read_fds);

    if (argc < 3) {
        usage(argv[0]);
    }

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp < 0, "socket");

    FD_SET(sockfd_tcp, & read_fds);
    fdmax = sockfd_tcp;
    FD_SET(0, & read_fds);

    strcpy(id_client, argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], & serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd_tcp, (struct sockaddr * ) & serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    n = send(sockfd_tcp, id_client, strlen(id_client), 0);
    DIE(n < 0, "send");
    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, & tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        /******     CITESTE DE LA TASTATURA     ********/
        if (FD_ISSET(0, & tmp_fds)) { 
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            topic aux_topic;
            char msg_read[20];
            strcpy(msg_read, buffer);
            // EXIT
            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            } 
            // SUBSCRIBE
            else if (strncmp(buffer, "subscribe", 9) == 0) {
                char * token;
                char * rest = buffer;
                int index = 0;
                char sub[15];
                while ((token = __strtok_r(rest, " ", & rest))) {
                    if (index == 0) {
                        strcpy(sub, token);
                        ++index;
                    } else if (index == 1) {
                        strcpy(aux_topic.topic, token);
                        ++index;
                    } else
                        aux_topic.SF = atoi(token);
                }

               
                    n = send(sockfd_tcp, msg_read, strlen(msg_read), 0);
                    DIE(n < 0, "send");
                   
                
                    printf("subscribed %s\n", aux_topic.topic);
                
                
            }
            // UNSUBSCRIBE
             else if (strncmp(buffer, "unsubscribe", 11) == 0) {

                char *rest = strtok(buffer + 12, "\n");
        
                    n = send(sockfd_tcp, msg_read, strlen(msg_read), 0);
                    DIE(n < 0, "send");
                    
                    printf("unsubscribed %s\n", rest);
                
                
            }
        }
        //  PRIMESTE MESAJE
         else if(FD_ISSET(sockfd_tcp, &tmp_fds)){
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd_tcp, buffer, BUFLEN, 0);
            DIE(n < 0, "recv");
            if (n == 0) //exit de la server
                break;
            else {
              printf("%s\n", buffer);   //afiseaza mesaj
            }
        }
    }

    close(sockfd_tcp);
    return 0;
}