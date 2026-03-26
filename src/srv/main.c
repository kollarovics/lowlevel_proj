#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "file.h"
#include "parse.h"
#include "srvpoll.h"

clientstate_t states[MAX_CLIENTS] = {0};

void print_usage(char *argv[]) {
    printf("Usage: %s [-n] [-f filepath] -p [port number]\n", argv[0]);
    printf("Options:\n");
    printf("\t  -n: Create a new database file\n");
    printf("\t  -f: Specify the filepath to use\n");
    printf("\t  -p: Specify port to listen to\n");
}

void poll_loop(int port, struct dbheader_t *header, struct employee_t *employees, int dbfd) {
    int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int opt = 1;

    init_clients(states);

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    //bind
    if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    //listen
    if (listen(listen_fd, 10) == -1) {
       perror("listen");
       exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
          int ii = 1;

          // Add active conections to the read set
          for (int i = 0; i < MAX_CLIENTS; i++) {
              if (states[i].state != -1) {
                  fds[ii].fd = states[i].fd;
                  fds[ii].events = POLLIN;
                  ii++;
              }
          }

          int n_events = poll(fds, nfds, -1);
          if (n_events == -1) {
              perror("poll");
              exit(EXIT_FAILURE);
          }

          if (fds[0].revents & POLLIN) {
              if ((conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
                  perror("accept");
                  continue;
              }

              printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
              freeSlot = find_free_slot(states);
              if (freeSlot == -1) {
                    printf("No free slots, closing incoming conection\n");
					close(conn_fd);
              } else {
                    states[freeSlot].fd = conn_fd;
                    states[freeSlot].state = STATE_HELLO;
					nfds++;
                    printf("Slot %d has fd %d\n", freeSlot, states[freeSlot].fd);
              }
              n_events--;
          }

          for (int i = 1; i <= nfds && n_events > 0; i++) {
              if (fds[i].revents & POLLIN) {
                  n_events--;
                  int fd = fds[i].fd;
                  int slot = find_slot_by_fd(states, fd);
                  printf("Got slot %d for fd %d\n", slot, fd);
                  ssize_t bytes_read = read(fd, &states[slot].buff, sizeof(states[slot].buff));
                  if (bytes_read <= 0) {
                      close(fd);
                      if (slot == -1) {
                         printf("Tried to close slot that does not exists\n");
                      } else {
                         states[slot].fd = -1;
                         states[slot].state = STATE_DISCONNECTED;
                         printf("Connection closed...\n");
                         nfds--;
                      }

                  } else {
                      // printf("Received %zd bytes:\n", bytes_read);
                      // for (ssize_t i = 0; i < bytes_read; i++) {
                      //     printf("%02X ", (unsigned char)states[slot].buff[i]);
                      // }
                      // printf("\n");
                      handle_client_fsm(header, &employees, &states[slot], dbfd);
                  }
              }
          }
    }

}

int main(int argc, char *argv[]) {
    int currCase;
    bool newFile = false;
    char *filepath = NULL;
    int dbfd = -1;
    struct dbheader_t *header = NULL;
    struct employee_t *employees = NULL;
    char *portarg = NULL;
    unsigned short port = 0;


    while ((currCase = getopt(argc, argv, "nf:p:")) != -1) {
        switch (currCase) {
            case 'n':
                newFile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'p':
                portarg = optarg;
                port = atoi(portarg);
                if (port == 0) {
                    printf("Invalid port number: %s\n", portarg);
                }
                break;
            case '?':
                printf("Unknown option: %c\n", optopt);
                break;
            default:
                return STATUS_ERROR;
        }
    }

    if (filepath == NULL) {
        printf("No filepath specified\n");
        print_usage(argv);
        return STATUS_SUCCESS;
    }

    if (port == 0) {
        printf("No port specified\n");
        print_usage(argv);
        return STATUS_SUCCESS;
    }


    if (newFile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Error creating database file\n");
            return STATUS_ERROR;
        }

        if (create_db_header(/*dbfd, */&header) == STATUS_ERROR) {
            printf("Error creating database header\n");
            return STATUS_ERROR;
        }
    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Error opening database file\n");
            return STATUS_ERROR;
        }
        if (validate_db_header(dbfd, &header) == STATUS_ERROR) {
            printf("Error validating database header\n");
            return STATUS_ERROR;
        }
    }

    if (read_employees(dbfd, header, &employees) != STATUS_SUCCESS) {
           printf("Error reading employees\n");
           return STATUS_ERROR;
    }

    poll_loop(port, header, employees, dbfd);

    output_file(dbfd, header, employees);
    return 0;
}
