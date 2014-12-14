#include "upload.h"
#include "hmac_sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_THREADS 10

static void usage();
static void *upload_thread();
static void send_data();

static char *filename, *account, *key, *container, *vhd;
static char *decoded_sign_key;
static int key_len;

static pthread_mutex_t length_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t length_condition = PTHREAD_COND_INITIALIZER, length_condition_r = PTHREAD_COND_INITIALIZER;
static unsigned long buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_LONG__], len, idx;
static unsigned long main_buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_LONG__], main_len, main_idx;

static unsigned long count, request_count, send_count;

static int quit;

int main(int argc, char **argv)
{
    FILE *fp;

    pthread_t threads[MAX_THREADS];
    
    unsigned long read_buffer[512 / __SIZEOF_LONG__];
    unsigned long total_pages = 0;
    int i;
    time_t begin_t, end_t;
    unsigned long is_send, is_pre_send;

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
    decoded_sign_key = (char *) malloc(strlen(key));
    key_len = unbase64(key, decoded_sign_key, strlen(key));

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
        is_send = 0;
        for (i = 0; i < 512 / __SIZEOF_LONG__; i++) {
            if (read_buffer[i] != 0) {
                is_send = 1;
                break;
            }
        }
        
        if (is_send == 1) {
            main_idx++;
            if (is_pre_send == 1) {
                if (main_len < MAX_PAGES_PER_UPLOAD - 1) {
                    memcpy(main_buffer + main_len * 512 / __SIZEOF_LONG__, read_buffer, 512);
                    main_len++;
                } else if (main_len == MAX_PAGES_PER_UPLOAD - 1) {
                    memcpy(main_buffer + main_len * 512 / __SIZEOF_LONG__, read_buffer, 512);
                    main_len++;
                    send_data();
                    request_count++;
                    main_len = 0;
                }
            } else {
                memcpy(main_buffer + main_len * 512 / __SIZEOF_LONG__, read_buffer, 512);
                main_len++;
            }
        } else {
            if (main_len > 0) {
                send_data();
                request_count++;
                main_len = 0;
            }
            main_idx++;
        }
        
        is_pre_send = is_send;
        
        if (main_idx % 2048 == 0) {
            end_t = time(NULL);
            printf("Scaned: %.2f\% (%.2fMB/%.2fMB), Uploaded: %.2fMB in %lu/%lu requests, Average Speed: %.2fMB/S, Elapsed Time: %02d:%02d:%02d\r", main_idx * 100.0 / total_pages, main_idx / 2048.0 , total_pages / 2048.0, count / 2048.0, send_count, request_count, (count / 2048.0) / (end_t - begin_t), (end_t - begin_t) / 3600, (end_t - begin_t) % 3600 / 60, (end_t - begin_t) % 3600 % 60);
            fflush(stdout);
        }
    }
    
    quit = 1;
    
    pthread_mutex_lock(&length_mutex);
    pthread_cond_broadcast(&length_condition);
    pthread_mutex_unlock(&length_mutex);
    
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_t = time(NULL);
    printf("Scaned: %.2f\% (%.2fMB/%.2fMB), Uploaded: %.2fMB in %lu/%lu requests, Average Speed: %.2fMB/S, Elapsed Time: %02d:%02d:%02d\n", main_idx * 100.0 / total_pages, main_idx / 2048.0, total_pages / 2048.0, count / 2048.0, send_count, request_count, (count / 2048.0) / (end_t - begin_t), (end_t - begin_t) / 3600, (end_t - begin_t) % 3600 / 60, (end_t - begin_t) % 3600 % 60);
    fflush(stdout);

    azure_upload_cleanup();
    fclose(fp);
    free(decoded_sign_key);
}

static void usage()
{
    printf("Usage: vhd_upload <path> <account> <key> <container> <vhd>\n");
}

static void *upload_thread()
{
    unsigned long len_t = 0, idx_t = 0;
    struct upload_data updata;
    
    unsigned long process_count = 0;
    
    CURL *curl;
    
    curl = curl_easy_init();
    
    for(;;) {
        pthread_mutex_lock(&length_mutex);
        
        if (len == 0) {
            if (quit == 1) {
                pthread_mutex_unlock(&length_mutex);
                break;
            }
            pthread_cond_wait(&length_condition, &length_mutex);
            if (quit == 1) {
                pthread_mutex_unlock(&length_mutex);
                break;
            }
        }       
        
        if (len > 0) {
            len_t = len;
            idx_t = idx;
            memcpy(updata.buffer, buffer, len_t * 512);
            send_count++;
            
            len = 0;
            pthread_cond_signal(&length_condition_r);
        }
        
        pthread_mutex_unlock(&length_mutex);
        
        if (len_t > 0) {
            azure_upload(curl, &updata, idx_t * 512, len_t * 512, account, decoded_sign_key, key_len, container, vhd);
        }

        if (quit == 1) {
            break;
        }
    }
    
    curl_easy_cleanup(curl);
}

static void send_data()
{
    pthread_mutex_lock(&length_mutex);
    
    if (len > 0) {
        pthread_cond_signal(&length_condition);
        pthread_cond_wait(&length_condition_r, &length_mutex);
    }
    
    if (len == 0 && main_len > 0) {
        len = main_len;
        idx = main_idx - main_len;
        memcpy(buffer, main_buffer, len * 512);
    
        pthread_cond_signal(&length_condition);
        
        count += main_len;
    }
    
    pthread_mutex_unlock(&length_mutex);
}
