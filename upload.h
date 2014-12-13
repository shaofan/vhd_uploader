#define MAX_PAGES_PER_UPLOAD 8

#ifndef __SIZEOF_INT__
#define __SIZEOF_INT__ sizeof(int)
#endif

struct upload_data {
    int data_length;
    int data_current;
    int buffer[MAX_PAGES_PER_UPLOAD * 512 / __SIZEOF_INT__];
};

void azure_upload_init();
int azure_upload(CURL *curl, struct upload_data *data, int begin, int length, char *account, char *key, char *container, char *vhd);
void azure_upload_cleanup();
