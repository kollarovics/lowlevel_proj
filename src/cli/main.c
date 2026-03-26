//
// Created by adrian on 3/4/26.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"


int send_hello(int fd) {
   char buff[4096] = {0};

   dbproto_hdr_t *hdr = (dbproto_hdr_t *)buff;
   hdr->type = htonl(MSG_HELLO_REQ);
   hdr->len = htons(1);

   dbproto_hello_req *hello = (dbproto_hello_req *) &hdr[1];
   hello->proto = htons(PROTO_VER);
   printf("Sending protocol version: %d\n", PROTO_VER);
   write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

   read(fd, buff, sizeof(buff));

   hdr->type = ntohl(hdr->type);
   hdr->len = ntohs(hdr->len);

   if (hdr->type == MSG_ERROR) {
        printf("Protocol mismatch\n");
        close(fd);
        return STATUS_ERROR;
   }
   printf("Server connected, protocol v1\n");
   return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *addarg = NULL;
    char *portarg = NULL;
    char *hostarg = NULL;
    unsigned short port = 0;

    int c;
    while ((c = getopt(argc, argv, "p:h:a")) != -1) {
         switch(c) {
             case 'a':
               addarg = optarg;
               break;
             case 'p':
                 portarg = optarg;
                 port = atoi(portarg);
                 break;
             case 'h':
                 hostarg = optarg;
                 break;
             case '?':
                 printf("Unknown option -%c\n", c);
             default:
                 return STATUS_ERROR;
         }
    }

    if (port == 0) {
        printf("Bad port: %s\n", portarg);
        return STATUS_ERROR;
    }

    if (hostarg == NULL) {
        printf("No host specified\n");
        return STATUS_ERROR;
    }

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);
    serverInfo.sin_addr.s_addr = inet_addr(hostarg);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return STATUS_ERROR;
    }

    if ((connect(fd, (struct sockaddr *) &serverInfo, sizeof(serverInfo))) == -1) {
          perror("connect");
          close(fd);
          return 0;
    }

    if (send_hello(fd) != STATUS_SUCCESS) {
        return STATUS_ERROR;
    }

    close(fd);
    return 0;
}