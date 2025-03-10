/**
*@file	   hash.h
*@brief    This part of program is HASH algorithm by hardware
*@author   wang chao
*@data     2021.1.23
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#ifndef DRV_HASH_H
#define DRV_HASH_H

#define HASH_RET_SUCCESS		0
#define HASH_RET_ENOINPUT		-1
#define HASH_RET_ELEN			-2
#define HASH_RET_ENOOUTPUT	-3
#define HASH_RET_ENOMEM		-4


/**
@brief      Calculate the hash value of SHA256   
@param[in]  *input:Pointer to store input data
@param[in]  ilen:Length of input data
@param[out] output[]:Array of SHA256 values    
*/
int drv_hash_sha256_ret( const unsigned char *input, unsigned int ilen, unsigned char output[32]);

/**
@brief      Calculate the hash value of SHA512 
@param[in]  *input:Pointer to store input data
@param[in]  ilen:Length of input data
@param[out] output[]:Array of SHA512 values    
*/
int drv_hash_sha512_ret( const unsigned char *input, unsigned int ilen, unsigned char output[64]);


int drv_sha256_vector(unsigned int num_elem, const unsigned char * addr[], const unsigned int * plength, unsigned char * output);
int drv_sha512_vector(unsigned int num_elem, const unsigned char * addr[], const unsigned int * plength, unsigned char * output);


#endif 

