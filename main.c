#include "upload.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif

#define MAX_THREADS 3

static void usage();
static void *upload_thread()
static void send_data();

static char *filename, *account, *key, *container, *vhd;

static pthread_mutex_t length_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t length_condition = PTHREAD_COND_INITIALIZER, length_condition_r = PTHREAD_COND_INITIALIZER;
static int buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_INT__], length, index;
static int main_buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_INT__], main_length, main_index;

static int count;

static bool quit = false;

int main(int argc, char **argv)
{
    FILE *fp;

    pthread_t threads[MAX_THREADS];
    
    int read_buffer[512 / __SIZEOF_INT__];
    int total_pages = 0;
    int i;
    time_t begin_t, end_t;
    bool is_send, is_pre_send;

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
    total_pages = ftell(fp);
    if (total_pages % 512 != 0) {
        printf("wrong file size\n");
        return(1);
    }
    total_pages /= 512;    
    fseek(fp, 0L, SEEK_SET);
    
    for (i = 0; i < MAX_THREADS; i++) {
         pthread_create(&threads[i], NULL, &upload_thread, NULL);
    }

    begin_t = time(NULL);
    while (!feof(fp)) {
        fread(read_buffer, 512, 1, fp);
        is_send = false;
        for (i = 0; i < 512 / __SIZEOF_INT__; i++) {
            if (read_buffer[i] != 0) {
                is_send = true;
                break;
            }
        }
        
        if (is_send) {
            if (is_pre_send) {
                if (main_length < MAX_PAGES_PER_UPLOAD - 1) {
                    memcpy(main_buffer + main_length * 512, read_buffer, 512);
                    main_length++;
                } else if (main_length == MAX_PAGES_PER_UPLOAD - 1) {
                    memcpy(main_buffer + main_length * 512, read_buffer, 512);
                    main_length++;
                    send_data();
                    main_length = 0;
                }
            }
        } else if (main_length > 0) {
            send_data();
            main_length = 0;
        }
        
        if (main_index % 100 == 0) {
            end_t = time(NULL);
            printf("Scaned: %.2f\% (%d/%d) Uploaded: %d pages Average Speed: %.2f KB/S Elapsed Time: %2d:%2d:%2d\r", main_index * 100.0 / total_pages, main_index, total_pages, count, (count / 2.0) / (end_t - begin_t), (end_t - begin_t) / 3600, (end_t - begin_t) % 3600 / 60, (end_t - begin_t) % 3600 % 60);
            fflush(stdout);
        }
        main_index++;
    }
    
    quit = true;
    pthread_cond_broadcast(&length_condition);
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_t = time(NULL);
    printf("Scaned: %.2f\% (%d/%d) Uploaded: %d pages Average Speed: %.2f KB/S Elapsed Time: %2d:%2d:%2d\n", main_index * 100.0 / total_pages, main_index, total_pages, count, (count / 2.0) / (end_t - begin_t), (end_t - begin_t) / 3600, (end_t - begin_t) % 3600 / 60, (end_t - begin_t) % 3600 % 60);
    fflush(stdout);

    azure_upload_cleanup();
    fclose(fp);
}

static void usage()
{
    printf("Usage: vhd_upload <path> <account> <key> <container> <vhd>\n");
}

static void *upload_thread()
{
    int length_t = 0, index_t = 0;
    struct upload_data updata;
    
    for(;;) {
        pthread_mutex_lock(&length_mutex);
        
        if (length == 0) {
            pthread_cond_wait(&length_condition, &length_mutex);
            if (quit) {
                break;
            }
        }       
        
        length_t = length;
        index_t = index;
        memcpy(updata.buffer, buffer, length_t);
        
        length = 0;
        pthread_cond_signal(&length_condition_r);
        
        pthread_mutex_unlock(&length_mutex);
        
        if (length_t > 0) {
            azure_upload((struct upload_data *)buffer_t, index_t * 512, length_t, account, key, container, vhd);
            length_t = 0;
        }
        if (quit) {
            break;
        }
    }
}

static void send_data()
{
    for(;;) {
        pthread_mutex_lock(&length_mutex);
        
        if (length > 0) {
            pthread_cond_wait(&length_condition_r, &length_mutex);
        }
        
        length = main_length;
        index = main_index;
        memcpy(buffer, main_buffer, length);
        
        pthread_cond_signal(&length_condition);
        
        count += main_length;
        break;
        
        pthread_mutex_unlock(&length_mutex);
    }
}