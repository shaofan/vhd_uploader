#Include <stdio.h>

int azure_cmp() {
    FILE *fp1, *fp2;
    int buffer1[512/__SIZEOF_INT__], buffer2[512/__SIZEOF_INT__];
    int i, iblock, count;

    fp1 = fopen("/cnnokia/restro.vhd", "r");
    fp2 = fopen("/cnnokia/testupload.vhd", "r");

    if (fp1 == NULL || fp2 == NULL) {
        fprintf(stderr, "Can't open files!\n");
        return(1);
    }

    iblock = 1;
    while (!feof(fp1) && !feof(fp2)) {
        fread(buffer1, 512, 1, fp1);
        fread(buffer2, 512, 1, fp2);
        for (i = 0; i < 512/__SIZEOF_INT__; i++) {
            if (buffer1[i] != buffer2[i]) {
                printf("Block %d NE\n", iblock);
                count++;
                break;
            }
        }
        iblock++;
    }

    printf("count = %d\n", count);
    fclose(fp1);
    fclose(fp2);
}

int azure_cmp_zero() {
    FILE *fp1;
    int buffer1[512/__SIZEOF_INT__];
    int i, count = 0;

    fp1 = fopen("/cnnokia/restro.vhd", "r");
    if (fp1 == NULL) {
        fprintf(stderr, "Can't open files!\n");
        return(1);
    }

    while (!feof(fp1)) {
        fread(buffer1, 512, 1, fp1);
        for (i = 0; i < 512/__SIZEOF_INT__; i++) {
            if (buffer1[i] != 0) {
                count++;
                break;
            }
        }
    }

    printf("count = %d\n", count);

    fclose(fp1);
}

int main() 
{
    azure_cmp();
}