//
// Created by adrian on 3/4/26.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>


#include "common.h"


void send_hello(int fd) {
   char buff[4096] = {0};

   dbproto_hdr_t *hdr = (dbproto_hdr_t *)buff;
   hdr->type = htonl(MSG_HELLO_REQ);
   hdr->len = htons(1);

   write(fd, buff, sizeof(dbproto_hdr_t));
   printf("Sent hello, protocol v1\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_address>\n", argv[0]);
        return 0;
    }

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(8080);
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return STATUS_ERROR;
    }

    if (connect(fd, (struct sockaddr *) &serverInfo, sizeof(serverInfo)) == -1) {
          perror("connect");
          close(fd);
          return 0;
    }

    send_hello(fd);
    close(fd);

    return 0;
}