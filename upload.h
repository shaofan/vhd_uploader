#include <curl/curl.h>

#define MAX_PAGES_PER_UPLOAD 4096
//#define MAX_PAGES_PER_UPLOAD 8

#ifndef __SIZEOF_LONG__
#define __SIZEOF_LONG__ sizeof(unsigned long)
#endif

struct upload_data {
    unsigned long data_length;
    unsigned long data_current;
    unsigned long buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_LONG__];
};

void azure_upload_init();
int azure_upload(CURL *curl, struct upload_data *data, unsigned long begin, unsigned long length, char *account, char *key, int key_len, char *container, char *vhd);
void azure_upload_cleanup();
