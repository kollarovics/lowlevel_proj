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
*/

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *addstring) {
    printf("Adding employee: %s\n", addstring);
    char *name = strtok(addstring, ",");
    char *address = strtok(NULL, ",");
    char *hours = strtok(NULL, ",");

    printf("Name: %s, Address: %s, Hours: %s\n", name, address, hours);

    strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));
    strncpy(employees[dbhdr->count-1].address, address, sizeof(employees[dbhdr->count-1].address));

    unsigned int hours_int = atoi(hours);
    employees[dbhdr->count-1].hours = htonl(hours_int);

    return STATUS_SUCCESS;
}


int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Bad file decriptor from user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;
    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        printf("Error allocating memory\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count * sizeof(struct employee_t));
    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }
    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Bad file decriptor from user\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->count = htons(dbhdr->count);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + realcount * sizeof(struct employee_t));
    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->version = htons(dbhdr->version);

    lseek(fd, 0, SEEK_SET);
    write(fd, dbhdr, sizeof(struct dbheader_t));

    for (int i = 0; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        write(fd, &employees[i], sizeof(struct employee_t));
    }

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
    return STATUS_SUCCESS;
}


