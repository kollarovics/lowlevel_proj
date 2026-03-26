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

void fsm_reply_add_err(clientstate_t *client, dbproto_hdr_t *hdr) {
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

void fsm_reply_add(clientstate_t *client, dbproto_hdr_t *hdr) {
    hdr->type = htonl(MSG_EMPLOYEE_ADD_RESP);
    hdr->len = htons(0);
    dbproto_hello_resp *resp = (dbproto_hello_resp *) &hdr[1];
    resp->proto = htons(PROTO_VER);
    write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp));
}


void send_employees(struct dbheader_t *dbhdr, struct employee_t **employeeptr, clientstate_t *client) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buff;
    hdr->type = htonl(MSG_EMPLOYEE_LIST_RESP);
    hdr->len = htons(dbhdr->count);

    write(client->fd, hdr, sizeof(dbproto_hdr_t));

    dbproto_employee_list_resp *employee = (dbproto_employee_list_resp*)&hdr[1];

    struct employee_t *employees = *employeeptr;

    int i = 0;
    for (; i < dbhdr->count; i++) {
        strncpy(employee->name, employees[i].name, sizeof(employee->name));
        strncpy(employee->address, employees[i].address, sizeof(employee->address));
        employee->hours = htonl(employees[i].hours);
        write(client->fd, employee, sizeof(dbproto_employee_list_resp));
    }
}

void handle_client_fsm(struct dbheader_t *header, struct employee_t **employees, clientstate_t *client, int dbfd) {
    printf("Handling client FSM\n");
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buff;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    printf("Got message from client, type: %d, len: %d, client state: %d\n", hdr->type, hdr->len, client->state);

    if (client->state == STATE_MSG) {
        if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
            dbproto_employee_add_req *employee = (dbproto_employee_add_req *) &hdr[1];
            printf("Got employee add request: %s\n", employee->data);
            // printf("%.32s\n", (char *)employee->data);
            if (add_employee(header, employees, employee->data) != STATUS_SUCCESS) {
                fsm_reply_add_err(client, hdr);
                return;
            } else {
                fsm_reply_add(client, hdr);
                output_file(dbfd, header, *employees);
            }
        } else if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
            send_employees(header, employees, client);
        }
    }

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
        printf("Client state change to STATE_MSG\n");

     }


}