#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    char *temp;
    char *buffer;
    long flen;
    char *fname;
    char pname[1024];

    if ( argc == 2 ) {
        temp = argv[1];
        fname = strrchr(temp,'/')+1;
        strcpy(pname, fname);
        char *dot = strchr(pname, '.');
        while (dot != NULL) {
            *dot = '_';
            dot = strchr(pname, '.');
        }
    } else {
        printf("Filename not supplied\n");
        return 1;
    }

    fp = fopen(temp, "rb");
    fseek(fp, 0, SEEK_END);
    flen = ftell(fp);
    rewind(fp);

    buffer = (char *)malloc((flen + 1) * sizeof(char));
    fread(buffer, flen, 1, fp);
    fclose(fp);

    printf("\n//File: %s, Size: %lu\n", fname, flen);
    printf("#define %s_len %lu\n", pname, flen);
    printf("const unsigned char %s[] = {\n", pname);
    long i;
    for (i = 0; i < flen; i++) {
        printf(" 0x%02X", (unsigned char)(buffer[i]));
        if (i < (flen - 1)) {
            printf(",");
        }
        if ((i % 16) == 15) {
            printf("\n");
        }
    }
    printf("\n};\n\n");
    free(buffer);
    return 0;
}
