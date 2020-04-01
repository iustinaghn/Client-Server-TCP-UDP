#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <limits.h>
#include <inttypes.h>
#include "lib.h"

void usage(char *file)
{
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd_tcp, newsockfd_tcp, sockfd_udp, portno;
    char buffer[BUFLEN], aux[BUFLEN];
    unsigned char auxbuf[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr, sock;
    int n, i, j, ret;
    socklen_t clilen, sklen;
    char id_client[11];
    client all_clients[BUFLEN];
    int clients_num_all = 1, topic_num = 1, msg_num = 1;
    msg all_messages[BUFLEN];

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar
    fd_set aux_fds;
    int fdmax; // valoare maxima fd din multimea read_fds

    if (argc < 2)
    {
        usage(argv[0]);
    }

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_ZERO(&aux_fds);
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);  //se asteapta conexiuni (udp)
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0); // se asteapta conexiuni (tcp)
    DIE(sockfd_tcp < 0, "socket");

    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    sklen = sizeof(sock);
    ret = bind(sockfd_tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");
    //ret = listen(sockfd_udp, MAX_CLIENTS);
    ret = listen(sockfd_tcp, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    ret = bind(sockfd_udp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(0, &read_fds);
    FD_SET(sockfd_tcp, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    fdmax = max(sockfd_tcp, sockfd_udp);

    while (1)
    {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        //se parcurge multimea de descriptori
        for (i = 0; i <= fdmax; i++)
        {

            if (FD_ISSET(i, &tmp_fds))
            {
                if (i == 0) //descriptorul serverului
                {
                    // se primeste de la tastatura exit
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);
                    if (strncmp(buffer, "exit", 4) == 0)

                    {
                        for (j = 0; j <= fdmax; ++j)
                        {
                            if (FD_ISSET(j, &aux_fds))
                            {
                                sprintf(aux, "%s", "exit");
                                n = send(j, aux, strlen(aux), 0); //se trimite comanda exit catre toti clientii conectati
                                DIE(n < 0, "send");
                            }
                        }
                        //se inchid socketii
                        close(sockfd_tcp);
                        close(sockfd_udp);
                        return 0;
                    }
                }
                 /********  descriptorul clientului TCP  ***********/
                else if (i == sockfd_tcp)
                {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    clilen = sizeof(cli_addr);
                    newsockfd_tcp = accept(sockfd_tcp, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd_tcp < 0, "accept");

                    // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                    FD_SET(newsockfd_tcp, &read_fds);
                    FD_SET(newsockfd_tcp, &aux_fds);
                    if (newsockfd_tcp > fdmax)
                    {
                        fdmax = newsockfd_tcp;
                    }

                    memset(buffer, 0, BUFLEN);
                    n = recv(newsockfd_tcp, buffer, BUFLEN, 0); //citesc id-ul clientului
                    strcpy(id_client, buffer);
                    DIE(n < 0, "recv");
                    int on = 0;

                    /********* ADAUGAREA CLIENTULUI IN LISTA DE CLIENTI ********/

                    for (j = 0; j < clients_num_all; ++j)
                        /********   CLIENTUL ESTE DEJA CONECTAT ********/
                        if (strcmp(all_clients[j].id_client, buffer) == 0 && all_clients[j].connected == 1)
                        {
                            char msg[50];
                            sprintf(msg, "%s", "A subscriber with this name is already connected.");
                            n = send(fdmax, msg, strlen(msg), 0);
                            DIE(n < 0, "send");
                            close(newsockfd_tcp);
                            close(fdmax);
                            FD_CLR(fdmax, &read_fds);
                            FD_CLR(fdmax, &aux_fds);
                            on = 1;
                        }
                    if (on == 0)
                    {
                        /*********  CLIENTUL SE CONECTEAZA  **********/
                        printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                        int ok = 0;
                        for (j = 0; j < clients_num_all; ++j)
                        {
                            // daca a mai fost conectat, se cauta in lista si se seteaza connected = 1 (clientul este online)
                            if (strcmp(all_clients[j].id_client, buffer) == 0)
                            {
                                all_clients[j].port = newsockfd_tcp;
                                all_clients[j].connected = 1;
                                ok = 1;
                            }
                        }
                        // daca nu a mai fost conectat, se adauga in lista si se seteaza connected = 1 (clientul este online)
                        if (ok == 0)
                        {
                            ++clients_num_all;
                            strcpy(all_clients[clients_num_all - 2].id_client, buffer);
                            all_clients[clients_num_all - 2].port = newsockfd_tcp;
                            all_clients[clients_num_all - 2].connected = 1;
                        }
                    }

                        /******** TRIMITEREA MESAJELOR PRIMITE IN MODUL OFFLINE ********/
                    for (j = 0; j < clients_num_all; ++j)
                    {
                        for (int k = 0; k < topic_num; ++k)
                        {
                            // daca este abonat cu sf = 1 si este conectat
                            if (all_clients[j].topics[k].SF == 1 && all_clients[j].connected == 1)
                            {
                                for (int l = 0; l < msg_num; l++)
                                    for (int m = 0; m < all_messages[l].lport; ++m)
                                    {   //se cauta pentru fiecare mesaj daca, clientul curent este abonat la topic si daca da, se trimite
                                        if (all_messages[l].port[m] == all_clients[j].port && all_messages[l].sent == 0)
                                        {
                                            n = send(all_clients[j].port, all_messages[l].msg, strlen(all_messages[l].msg), 0);
                                            DIE(n < 0, "send");
                                            all_messages[l].sent = 1;   //un mesaj trimis se marcheaza cu 1 pt a nu fi retrimis
                                        }
                                    }
                            }
                        }
                    }
                }
                 /*********     descriptorul clientului UDP  *********/
                else if (i == sockfd_udp)
                {

                    clilen = sizeof(cli_addr);
                    memset(buffer, BUFLEN, 0);
                    int n = recvfrom(sockfd_udp, buffer, BUFLEN, 0, (struct sockaddr *)&cli_addr, &clilen); //se primesc mesajele
                    DIE(n < 0, "recvfrom");

                    char compute_message[BUFLEN];
                    int type = buffer[50];
                    char topic[50];
                    strcpy(topic, buffer);
                    uint32_t nr_int, nr_float;
                    uint16_t nr_short;
                    int k = 0;


                    memset(compute_message, BUFLEN, 0);
                    memset(auxbuf, BUFLEN, 0);
                    
                    //INT se afla la buffer + 52, buffer[51] -> octet de semn
                    memcpy(&nr_int, buffer + 52, 4);
                    memcpy(&nr_float, buffer + 52, 4);
                    memcpy(&nr_short, buffer + 51, 2);

                    //pentru unsigned int pe 32 de biti s-a folosit htonl
                    nr_int = htonl(nr_int);
                    nr_float = htonl(nr_float);
                    //pentru unsigned int pe 16 biti s-a folosit htons
                    nr_short = htons(nr_short);

                    /********   CONCATENAREA MESAJULUI  ********/
                    char var[12];
                    sprintf(var, "%d", ntohs(cli_addr.sin_port));
                    if (type == 3)
                    {
                        for (int j = 51; j <= BUFLEN; ++j)
                            auxbuf[k++] = buffer[j];
                    }
                    strcpy(compute_message, inet_ntoa(cli_addr.sin_addr));
                    strcat(compute_message, ":");
                    strcat(compute_message, var);
                    strcat(compute_message, " - ");
                    strcat(compute_message, topic);
                    strcat(compute_message, " - ");

                    char vari[10];

                    if (type == INT)
                    {
                        strcat(compute_message, "INT");
                        if (buffer[51] == 1)
                            nr_int = (-1) * nr_int;
                        sprintf(vari, "%d", nr_int);
                        strcat(compute_message, " - ");
                        strcat(compute_message, vari);
                    }
                    else if (type == SHORT_REAL)
                    {
                        strcat(compute_message, "SHORT_REAL");
                        float nr_short2 = nr_short;
                        nr_short2 /= 100;

                        if (buffer[51] == 1)
                            nr_short2 = (-1) * nr_short2;
                        sprintf(vari, "%.2f", nr_short2);
                        strcat(compute_message, " - ");
                        strcat(compute_message, vari);
                    }
                    else if (type == FLOAT)
                    {
                        float nr_float2 = nr_float;
                        int nr_dec = buffer[56];
                        int a = 10;

                        if (nr_dec == 0)
                            a = 1;

                        while (nr_dec > 1)
                        {
                            a *= 10;
                            --nr_dec;
                        }
                        nr_float2 /= a;
                        if (buffer[51] == 1)
                            nr_float2 = (-1) * nr_float2;
                        strcat(compute_message, "FLOAT");
                        strcat(compute_message, " - ");
                        sprintf(vari, "%.*f", buffer[56], nr_float2);
                        strcat(compute_message, vari);
                    }
                    else
                    {
                        strcat(compute_message, "STRING");
                        strcat(compute_message, " - ");
                        strcat(compute_message, auxbuf);
                    }

                    /********   TRIMITEREA MESAJULUI    *********/
                    for (j = 0; j < clients_num_all; ++j)
                    {

                        for (int k = 0; k < topic_num; ++k)
                        {
                            // daca e conectat si abonat, se trimite mesajul
                            if (strcmp(all_clients[j].topics[k].topic, topic) == 0 && all_clients[j].connected == 1 && all_clients[j].topics[k].subscribed == 1)
                            {
                                int n = send(all_clients[j].port, compute_message, strlen(compute_message), 0);
                                DIE(n < 0, "send");
                                memset(compute_message, 0, BUFLEN);
                            }
                            //daca e abonat, dar neconectat si sf = 1, se adauga mesajul intr-un vector de mesaje pentru a fi putea trimis mai tarziu
                            else if (strcmp(all_clients[j].topics[k].topic, topic) == 0 && all_clients[j].connected == 0 && all_clients[j].topics[k].subscribed == 1 && all_clients[j].topics[k].SF == 1)
                            {
                                strcpy(all_messages[msg_num].msg, compute_message);
                                all_messages[msg_num].port[all_messages[msg_num].lport] = all_clients[j].port;
                                all_messages[msg_num].sent = 0;
                                ++msg_num;
                            }
                        }
                    }
                }
                /********   CLIENTUL A PRIMIT TEXT DE LA TASTATURA    *********/
                else
                {
                    // s-au primit date pe unul din socketii de client,
                    // asa ca serverul trebuie sa le receptioneze
                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    //  EXIT
                    if (n == 0)
                    {
                        for (j = 0; j < clients_num_all; ++j)
                            if (all_clients[j].port == i)
                            {
                                strcpy(aux, all_clients[j].id_client);
                                all_clients[j].connected = 0;
                                all_clients[j].port = -1;
                            }
                        // conexiunea s-a inchis
                        printf("Client %s disconnected.\n", aux);
                        close(i);

                        // se scoate din multimea de citire socketul inchis
                        FD_CLR(i, &read_fds);
                        FD_CLR(i, &aux_fds);
                    }
                    //  SUBSCRIBE / UNSUBSCRIBE
                    else
                    {
                        topic aux_topic;

                        if (strncmp(buffer, "subscribe", 9) == 0)
                        {
                            char *token;
                            char *rest = buffer;
                            int index = 0;
                            char sub[15];
                            while ((token = __strtok_r(rest, " ", &rest)))
                            {
                                if (index == 0)
                                {
                                    strcpy(sub, token);
                                    ++index;
                                }
                                else if (index == 1)
                                {
                                    strcpy(aux_topic.topic, token);
                                    ++index;
                                }
                                else
                                    aux_topic.SF = atoi(token);
                            }
                            int ok = 0;

                            for (j = 0; j < topic_num; ++j)
                            {
                                 //daca nu a mai fost abonat la acest topic, se adauga topicul
                                if (strcmp(all_clients[i - 5].topics[j].topic, aux_topic.topic) != 0)
                                {
                                    ++topic_num;
                                    memcpy(all_clients[i - 5].topics[topic_num - 1].topic, aux_topic.topic, strlen(aux_topic.topic));
                                    all_clients[i - 5].topics[topic_num - 1].SF = aux_topic.SF;
                                    all_clients[i - 5].topics[topic_num - 1].subscribed = 1;
                                }
                                //daca a mai fost abonat, dar s-a reabonat, se rescrie SF in cazul in care s-a schimbat
                                else
                                {
                                    if (all_clients[i - 5].topics[j].subscribed == 0)
                                    {
                                        all_clients[i - 5].topics[j].SF = aux_topic.SF;
                                        all_clients[i - 5].topics[j].subscribed = 1;
                                    }
                                }
                            }
                        }
                        else if (strncmp(buffer, "unsubscribe", 11) == 0)
                        {
                            char *rest = strtok(buffer + 12, "\n"); //topic
                           

                            for (int j = 1; j < topic_num; ++j)
                            {
                                //daca s-a dezabonat de la un topic, se marcheaza cu 0
                                if (strcmp(all_clients[i - 5].topics[j].topic, aux_topic.topic) == 0)
                                {
                                    all_clients[i - 5].topics[j].subscribed = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //inchiderea socketilor
    close(sockfd_tcp);
    close(sockfd_udp);
    return 0;
}