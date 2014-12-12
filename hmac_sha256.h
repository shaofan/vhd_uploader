void hmac_sha256( const unsigned char *text, int text_len, const unsigned char *key, int key_len, void *digest);
char *base64( const unsigned char *src, size_t sz );
char *unbase64(unsigned char *input, int length);
void printdump( const char *buffer, size_t sz );
