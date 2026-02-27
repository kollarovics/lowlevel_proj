#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

/*
void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {

}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {

}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {

}*/

int output_file(int fd, struct dbheader_t *dbhdr/*, struct employee_t *employees*/) {
    if (fd < 0) {
        printf("Bad file decriptor from user\n");
        return STATUS_ERROR;
    }

    dbhdr->count = htons(dbhdr->count);
    dbhdr->filesize = htonl(dbhdr->filesize);
    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET);
    write(fd, dbhdr, sizeof(struct dbheader_t));

    return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Bad file decriptor from user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("Error allocating memory");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("Error reading header");
        free(header);
        return STATUS_ERROR;
    };

    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);
    header->count = ntohs(header->count);
    header->version = ntohs(header->version);

    if (header->magic != HEADER_MAGIC) {
        printf("Bad magic number\n");
        free(header);
        return STATUS_ERROR;
    }

    if (header->version != 0x01) {
        printf("Improper version number\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat st = {0};
    fstat(fd, &st);
    if (header->filesize != st.st_size) {
        printf("Corrupted DB file\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(/*int fd,*/ struct dbheader_t **headerOut) {
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        perror("Error allocating memory");
        return STATUS_ERROR;
    }

    header->magic = HEADER_MAGIC;
    header->version = 0x01;
    header->count = 0;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
}


