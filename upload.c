#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/x509.h>
#include <pthread.h>

#include "hmac_sha256.h"
#include "upload.h"

static pthread_mutex_t openssl_mutex = PTHREAD_MUTEX_INITIALIZER;

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    struct upload_data *updata;
    char *pdata;

    updata = (struct upload_data *) stream;
    pdata = (char *) updata->buffer;

    if (updata->data_current == updata->data_length - 1) {
        return 0;
    }

    size_t count = size * nmemb;
    if (updata->data_length - updata->data_current < count) {
        count = updata->data_length - updata->data_current;
    }

    memcpy(ptr, pdata + updata->data_current, count);
    updata->data_current += count;
    return count;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{

}

void azure_upload_init()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

int azure_upload(CURL *curl, struct upload_data *data, unsigned long begin, unsigned long length, char *account, char *key, int key_len, char *container, char *vhd)
{
    CURLcode res;

    struct curl_slist *headerlist = NULL;
    char date_str[64], header_date_str[64];
    //char header_length_str[64];
    unsigned char header_authorization_str[256];
    unsigned char range_str[64], header_range_str[64];
    unsigned char sign_str[1024], signed_str[1024], *base64_str;
    unsigned char url[512];

    time_t current_time;
    struct tm *current_tm;

    if (data == NULL || length == 0 || length % 512 != 0 || begin % 512 != 0) {
        return 1;
    }

    memset(date_str, 0x0, sizeof date_str);
    memset(header_date_str, 0x0, sizeof header_date_str);
    //memset(header_length_str, 0x0, sizeof header_length_str);
    memset(header_authorization_str, 0x0, sizeof header_authorization_str);
    memset(range_str, 0x0, sizeof range_str);
    memset(header_range_str, 0x0, sizeof header_range_str);
    memset(sign_str, 0x0, sizeof sign_str);
    memset(signed_str, 0x0, sizeof signed_str);
    memset(url, 0x0, sizeof url);

    data->data_length = length;
    data->data_current = 0;

    if (curl) {
        pthread_mutex_lock(&openssl_mutex);
        time(&current_time);
        current_tm = gmtime(&current_time);
        strftime(date_str, sizeof date_str, "%a, %d %b %Y %T GMT", current_tm);
        sprintf(header_date_str, "Date: %s", date_str);
        headerlist = curl_slist_append(headerlist, header_date_str);
        headerlist = curl_slist_append(headerlist, "x-ms-version: 2014-02-14");
        sprintf(range_str, "bytes=%lu-%lu", begin, begin + length - 1);
        sprintf(header_range_str, "Range: %s", range_str);
        headerlist = curl_slist_append(headerlist, header_range_str);
        //sprintf(header_length_str, "Content-Length: %d", length);
        //headerlist = curl_slist_append(headerlist, header_length_str);
        headerlist = curl_slist_append(headerlist, "x-ms-page-write: update");
        headerlist = curl_slist_append(headerlist, "Content-Type: application/octet-stream");

        sprintf(sign_str, "PUT\n\n\n%lu\n\napplication/octet-stream\n%s\n\n\n\n\n%s\nx-ms-page-write:update\nx-ms-version:2014-02-14\n/%s/%s/%s\ncomp:page", length, date_str, range_str, account, container, vhd);

        hmac_sha256(sign_str, strlen(sign_str), key, key_len, signed_str);
        base64_str = base64(signed_str, SHA256_DIGEST_LENGTH);

        sprintf(header_authorization_str, "Authorization: SharedKey %s:%s", account, base64_str);
        headerlist = curl_slist_append(headerlist, header_authorization_str);

        sprintf(url, "https://%s.blob.core.chinacloudapi.cn/%s/%s?comp=page", account, container, vhd);
        pthread_mutex_unlock(&openssl_mutex);

        curl_easy_reset(curl);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void *)data);
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, length);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: url = %s, sign_str = %s, %s\n", url, sign_str, curl_easy_strerror(res));
        } else {
            //fprintf(stderr, "curl_easy_perform() success: url = %s, sign_str = %s, %s\n", url, sign_str, curl_easy_strerror(res));
        }

        curl_slist_free_all(headerlist);
    }
}

int azure_put_pageblob(char *account, char *key, int key_len, char *container, char *vhd, unsigned long length)
{
    CURL *curl;
    CURLcode res;

    struct curl_slist *headerlist = NULL;
    char date_str[64], header_date_str[64];
    char header_length_str[64];
    unsigned char header_authorization_str[256];
    unsigned char sign_str[1024], signed_str[1024], *base64_str;
    unsigned char url[512];

    time_t current_time;
    struct tm *current_tm;

    memset(date_str, 0x0, sizeof date_str);
    memset(header_date_str, 0x0, sizeof header_date_str);
    memset(header_length_str, 0x0, sizeof header_length_str);
    memset(header_authorization_str, 0x0, sizeof header_authorization_str);
    memset(sign_str, 0x0, sizeof sign_str);
    memset(signed_str, 0x0, sizeof signed_str);
    memset(url, 0x0, sizeof url);

    curl = curl_easy_init();
    if (curl) {
        pthread_mutex_lock(&openssl_mutex);
        time(&current_time);
        current_tm = gmtime(&current_time);
        strftime(date_str, sizeof date_str, "%a, %d %b %Y %T GMT", current_tm);
        sprintf(header_date_str, "Date: %s", date_str);
        headerlist = curl_slist_append(headerlist, header_date_str);

        sprintf(header_length_str, "x-ms-blob-content-length: %lu", length);
        headerlist = curl_slist_append(headerlist, header_length_str);

        headerlist = curl_slist_append(headerlist, "x-ms-version: 2014-02-14");
        headerlist = curl_slist_append(headerlist, "x-ms-blob-type: PageBlob");
        headerlist = curl_slist_append(headerlist, "Content-Type: application/octet-stream");

        sprintf(header_length_str, "x-ms-blob-content-length:%lu", length);
        sprintf(sign_str, "PUT\n\n\n%d\n\napplication/octet-stream\n%s\n\n\n\n\n\n%s\nx-ms-blob-type:PageBlob\nx-ms-version:2014-02-14\n/%s/%s/%s", 0, date_str, header_length_str, account, container, vhd);

        hmac_sha256(sign_str, strlen(sign_str), key, key_len, signed_str);
        base64_str = base64(signed_str, SHA256_DIGEST_LENGTH);

        sprintf(header_authorization_str, "Authorization: SharedKey %s:%s", account, base64_str);
        headerlist = curl_slist_append(headerlist, header_authorization_str);

        sprintf(url, "https://%s.blob.core.chinacloudapi.cn/%s/%s", account, container, vhd);
        pthread_mutex_unlock(&openssl_mutex);

        curl_easy_reset(curl);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        //curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        //curl_easy_setopt(curl, CURLOPT_READDATA, (void *)data);
        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, 0);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: url = %s, sign_str = %s, %s\n", url, sign_str, curl_easy_strerror(res));
        } else {
            //fprintf(stderr, "curl_easy_perform() success: url = %s, sign_str = %s, %s\n", url, sign_str, curl_easy_strerror(res));
        }

        curl_slist_free_all(headerlist);
        curl_easy_cleanup(curl);
    }
}

void azure_upload_cleanup()
{
    curl_global_cleanup();
}
