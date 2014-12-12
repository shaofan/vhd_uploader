#include "upload.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif

void usage();

int main(int argc, char **argv)
{
    char *filename, *account, *key, *container, *vhd;
    FILE *fp;
    int buffer[512/__SIZEOF_INT__];
    int index = 0, total_pages = 0;
    int i, count;
    time_t begin_t, end_t;

    if (argc != 6) {
        printf("wrong number of arguments\n");
        usage();
        return(1);
    }

    filename = argv[1];
    account = argv[2];
    key = argv[3];
    container = argv[4];
    vhd = argv[5];

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Can't open file %s!\n", filename);
        return(1);
    }

    azure_upload_init();
    
    fseek(fp, 0L, SEEK_END);
    total_pages = ftell(fp) / 512;
    fseek(fp, 0L, SEEK_SET);

    begin_t = time(NULL);
    while (!feof(fp)) {
        fread(buffer, 512, 1, fp);
        for (i = 0; i < 512/__SIZEOF_INT__; i++) {
            if (buffer[i] != 0) {
                azure_upload((char *)buffer, index * 512, 512, account, key, container, vhd);
                count++;
                break;
            }
        }
        if (index % 100 == 0) {
            end_t = time(NULL);
            printf("Total %d pages, scaned %d pages, uploaded %d pages, %f KBps\r", total_pages, index, count, (count / 2.0) / (end_t - begin_t));
            fflush(stdout);
        }
        index++;
    }
    end_t = time(NULL);
    printf("Total %d pages, scaned %d pages, uploaded %d pages, %f KBps\r", total_pages, index, count, (count / 2.0) / (end_t - begin_t));
    fflush(stdout);

    azure_upload_cleanup();
    fclose(fp);
}

void usage()
{
    printf("Usage: vhd_upload <path> <account> <key> <container> <vhd>\n");
}
