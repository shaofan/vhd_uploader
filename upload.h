void azure_upload_init();
int azure_upload(char *data, int begin, int length, char *account, char *key, char *container, char *vhd);
void azure_upload_cleanup();
