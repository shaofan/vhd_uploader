SOURCES=./main.c ./upload.h ./upload.c ./hmac_sha256.h ./hmac_sha256.c
CFILES=./main.c ./upload.c ./hmac_sha256.c
vhd_upload: $(SOURCES)
	cc -o $@ -lcrypto -lcurl -lpthread $(CFILES)