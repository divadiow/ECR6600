#ifndef HAL_HASH_H
#define HAL_HASH_H


int hal_hash_sha256(const unsigned char *input, unsigned int ilen, unsigned char output[32]);
int hal_hash_sha512(const unsigned char *input, unsigned int ilen, unsigned char output[64]);


#endif /*HAL_HASH_H*/


