#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/x509.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

void
hmac_sha256(
    const unsigned char *text,      /* pointer to data stream        */
    int                 text_len,   /* length of data stream         */
    const unsigned char *key,       /* pointer to authentication key */
    int                 key_len,    /* length of authentication key  */
    void                *digest)    /* caller digest to be filled in */
{
    unsigned int result_len;
    unsigned char result[EVP_MAX_MD_SIZE];

    HMAC(EVP_sha256(),
         key, strlen(key),
         text, strlen(text),
         result, &result_len);
    memcpy(digest, result, result_len);
}

void
printdump( const char *buffer, size_t sz )
{
    int             i, c;
    unsigned char   buf[80];

    for ( i = 0; (unsigned)i < sz; i++ ) {
        if ( (i != 0) && (i % 16 == 0) ) {
            buf[16] = 0;
            fprintf( stderr, "    %s\n", buf );
        }
        if ( i % 16 == 0 )
            fprintf( stderr, "%08x:", &buffer[i] );
        c = buffer[i] & 0xFF;
        if ( (c >= ' ') && (c <= 0x7E) )
            buf[i % 16] = (unsigned char)c;
        else
            buf[i % 16] = '.';
        fprintf( stderr, " %02x", c & 0xFF );
    }
    if ( i % 16 == 0 )
        buf[16] = 0;
    else {
        buf[i % 16] = 0;
        for ( i = i % 16; i < 16; i++ )
            fputs( "   ", stderr );
    }
    fprintf( stderr, "    %s\n", buf );
}

static char b[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char    *
base64( const unsigned char *src, size_t sz )
{
    unsigned char               *pp, *p, *q;
    static unsigned char *qq = NULL;
    size_t                      i, safe = sz;

    if ( qq ) {
        free( qq );
        qq = NULL;
    }
    if ( !src || (sz == 0) )
        return ( NULL );

    if ( (sz % 3) == 1 ) {
        p = (unsigned char *)malloc( sz + 2 );
        if ( !p )
            return ( NULL );
        memcpy( p, src, sz );
        p[sz] = p[sz + 1] = '=';
        sz += 2;
    }
    else if ( (sz % 3) == 2 ) {
        p = (unsigned char *)malloc( sz + 1 );
        if ( !p )
            return ( NULL );
        memcpy( p, src, sz );
        p[sz] = '=';
        sz++;
    }
    else
        p = (unsigned char *)src;

    q = (unsigned char *)malloc( (sz / 3) * 4 + 2 );
    if ( !q ) {
        if ( p != src )
            free( p );
        return ( NULL );
    }

    pp = p;
    qq = q;
    for ( i = 0; i < sz; i += 3 ) {
        q[0] = b[(p[0] & 0xFC) >> 2];
        q[1] = b[((p[0] & 0x03) << 4) | ((p[1] & 0xF0) >> 4)];
        q[2] = b[((p[1] & 0x0F) << 2) | ((p[2] & 0xC0) >> 6)];
        q[3] = b[p[2] & 0x3F];
        p += 3;
        q += 4;
    }
    *q = 0;
    if ( (safe % 3) == 1 ) {
        *(q - 1) = '=';
        *(q - 2) = '=';
    }
    if ( (safe % 3) == 2 )
        *(q - 1) = '=';

    if ( pp != src )
        free( pp );
    return ( (char *)qq );
}

char *unbase64(unsigned char *input, int length)
{
  BIO *b64, *bmem;

  char *buffer = (char *)malloc(length);
  memset(buffer, 0, length);

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new_mem_buf(input, length);
  bmem = BIO_push(b64, bmem);

  BIO_read(bmem, buffer, length);

  BIO_free_all(bmem);

  return buffer;
}

int main_t() 
{
  char signed_str[256], *base64_str;
  const char key_str[] = "key";
  const char sign_str[] = "PUT\nPUT";

  memset(signed_str, 0x0, sizeof signed_str);

  printf("sign_str = [%s]\n", sign_str);
  hmac_sha256(sign_str, strlen(sign_str), key_str, strlen(key_str), signed_str);
  printdump(signed_str, sizeof signed_str);
  base64_str = base64(signed_str, SHA256_DIGEST_LENGTH);
  printf("base64_str = [%s]\n", base64_str);
}
