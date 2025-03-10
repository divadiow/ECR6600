#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mbedtls/entropy_poll.h"
#include "oshal.h"


//#ifndef MIN
//   #define MIN(x,y) ((x)<(y)?(x):(y))
//#endif

void trs_fill_random(void *buf, size_t len)
{
    unsigned char *buf_bytes = (unsigned char *)buf;
    while (len > 0) {
        unsigned int word = os_random();
        unsigned int to_copy = MIN(sizeof(word), len);
        memcpy(buf_bytes, &word, to_copy);
        buf_bytes += to_copy;
        len -= to_copy;
    }
}

int mbedtls_hardware_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    trs_fill_random(output, len);
    *olen = len;
    return 0;
}
