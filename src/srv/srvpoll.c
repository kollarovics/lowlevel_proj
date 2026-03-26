//
// Created by adrian on 3/4/26.
//
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "srvpoll.h"

void init_clients(clientstate_t *states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].state = STATE_NEW;
        states[i].fd = -1;
        memset(&states[i].buff, '\0', BUFF_SIZE);
    }
}

int find_free_slot(clientstate_t *states) {
   for (int i = 0; i < MAX_CLIENTS; i++) {
       if (states[i].fd == -1) {
          return i;
       }
   }
    return -1;
}

int find_slot_by_fd(clientstate_t *states, int fd) {
   for (int i = 0; i < MAX_CLIENTS; i++) {
       if (states[i].fd == fd) {
           return i;
       }
   }
   return -1;
}

void fsm_reply_hello_err(clientstate_t *client, dbproto_hdr_t *hdr) {
   hdr->type = htonl(MSG_ERROR);
   hdr->len = htons(0);
   write(client->fd, hdr, sizeof(dbproto_hdr_t));
}

void fsm_reply_hello(clientstate_t *client, dbproto_hdr_t *hdr) {
   hdr->type = htonl(MSG_HELLO_RESP);
   hdr->len = htons(0);
   dbproto_hello_resp *resp = (dbproto_hello_resp *) &hdr[1];
   resp->proto = htons(PROTO_VER);
   write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp));
}

void handle_client_fsm(struct dbheader_t *header, struct employee_t *employees, clientstate_t *client) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buff;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    if (client->state == STATE_HELLO) {
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            printf("Didn't get hello in HELLO state\n");
            //TOTO: err
        }

        dbproto_hello_req *hello  = (dbproto_hello_req *) &hdr[1];
        hello->proto = ntohs(hello->proto);
        printf("Got hello from client, proto: %d\n", hello->proto);
        if (hello->proto != PROTO_VER) {
           printf("Bad protocol version\n");
           fsm_reply_hello_err(client, hdr);
           return;
        }
        fsm_reply_hello(client, hdr);
        client->state = STATE_MSG;
        printf("Client state change to STATE_MSG");

     }

     if (client->state == STATE_MSG) {
     }
}