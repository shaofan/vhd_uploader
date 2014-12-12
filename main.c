#include "upload.h"

#include <stdio.h>
#include <string.h>

void usage();

int main(int argc, char **argv)
{
  char *filename, *account, *key, *container, *vhd;
  FILE *fp;
  int buffer[512/__SIZEOF_INT__];
  int index = 0;
  int i, count;

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

  while (!feof(fp)) {
    fread(buffer, 512, 1, fp);
    for (i = 0; i < 512/__SIZEOF_INT__; i++) {
      if (buffer[i] != 0) {
	azure_upload((char *)buffer, index * 512, 512, account, key, container, vhd);	
	count++;
	printf("Uploaded page %d\n", index);
        break;
      }
    }
    if (index % 10 == 0) {
      printf("Scaned %d pages\r", index);
      fflush(stdout);
    }
    index++;
  }

  printf("count = %d\n", count);

  azure_upload_cleanup();

  fclose(fp);
}

void usage()
{
  printf("Usage: vhd_upload <path> <account> <key> <container> <vhd>\n");
}
