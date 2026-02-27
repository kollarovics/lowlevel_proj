#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s [-n] [-f filepath]\n", argv[0]);
    printf("Options:\n");
    printf("\t  -n: Create a new database file\n");
    printf("\t  -f: Specify the filepath to use\n");
}

int main(int argc, char *argv[]) {
    int currCase;
    bool newFile = false;
    char *filepath = NULL;
    int dbfd = -1;
    struct dbheader_t *header = NULL;

    while ((currCase = getopt(argc, argv, "nf:")) != -1) {
        switch (currCase) {
            case 'n':
                newFile = true;
                break;
            case 'f':
                filepath = optarg;
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


    if (newFile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Error creating database file\n");
            return STATUS_ERROR;
        }

        if (create_db_header(/*dbfd,*/ &header) == STATUS_ERROR) {
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


    printf("New file: %d\n", newFile);
    printf("Filepath: %s\n", filepath);
    output_file(dbfd, header);
    return 0;
}
