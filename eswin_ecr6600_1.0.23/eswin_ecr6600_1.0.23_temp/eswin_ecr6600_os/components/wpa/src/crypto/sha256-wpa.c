/*
 * SHA-256 hash implementation and interface functions
 * Copyright (c) 2003-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "sha256.h"
#include "crypto.h"
#include "hash.h"

/**
 * hmac_sha256_vector - HMAC-SHA256 over data vector (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash (32 bytes)
 * Returns: 0 on success, -1 on failure
 */


#define USE_HARD_HASH					0
#define USE_HARD_HASH_CHECK			0


#if USE_HARD_HASH_CHECK
unsigned char macdata[32];
int hash_cnt = 0;

#define DUMP_LINE_SIZE			8

extern int drv_sha256_vector(unsigned int num_elem, const unsigned char * addr[], const size_t * plength, unsigned char * output);
extern void cli_printf(const char *f, ...);

void cli_hexdump(const void * addr, unsigned int size)
{
	const unsigned char * c = addr;

	cli_printf("Dumping %u bytes from %p:\r\n", size, addr);

	while (size)
	{
		int i;

		for (i=0; i<DUMP_LINE_SIZE; i++)
		{
			if (i < size)
			{
				cli_printf("0x%02x, ", c[i]);
			}
			else
			{
				cli_printf("    ");
			}
		}

		cli_printf("\r\n");

		c += DUMP_LINE_SIZE;

		if (size <= DUMP_LINE_SIZE)
		{
			break;
		}

		size -= DUMP_LINE_SIZE;
	}
}


void cli_hexdump_int(const void * addr, unsigned int size)
{
	const unsigned int * c = addr;

	cli_printf("Dumping %u bytes from %p:\r\n", size, addr);

	while (size)
	{
		cli_printf("%08x%08x\r\n", *(c+1), *c);

		c+=2;
		size -= 8;
	}
}
#endif


int hmac_sha256_vector(const u8 *key, size_t key_len, size_t num_elem,
		       const u8 *addr[], const size_t *len, u8 *mac)
{
	unsigned char k_pad[64]; /* padding - key XORd with ipad/opad */
	unsigned char tk[32];
	const u8 *_addr[6];
	size_t _len[6], i;
	//cli_printf("wangc: entry %s\r\n", __func__);

	if (num_elem > 5) {
		/*
		 * Fixed limit on the number of fragments to avoid having to
		 * allocate memory (which could fail).
		 */
		return -1;
	}

        /* if key is longer than 64 bytes reset it to key = SHA256(key) */
        if (key_len > 64) {
#if USE_HARD_HASH
		if (drv_sha256_vector(1, &key, (unsigned int *)&key_len, tk) < 0)
			return -1;

#if USE_HARD_HASH_CHECK
		sha256_vector(1, &key, &key_len, macdata);
		if (memcmp(tk, macdata, 32)) {
			for (i=0; i<(1); i++) {
				cli_printf("index: %d\r\n", i);
				cli_hexdump(_addr[i], _len[i]);
			}
			cli_printf("hash: hard\r\n");
			cli_hexdump(tk, 32);

			cli_printf("hash: soft\r\n");
			cli_hexdump(macdata, 32);
		}
#endif

#else
		if (sha256_vector(1, &key, &key_len, tk) < 0)
			return -1;
#endif
		key = tk;
		key_len = 32;
        }

	/* the HMAC_SHA256 transform looks like:
	 *
	 * SHA256(K XOR opad, SHA256(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected */

	/* start out by storing key in ipad */
	os_memset(k_pad, 0, sizeof(k_pad));
	os_memcpy(k_pad, key, key_len);
	/* XOR key with ipad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x36;

	/* perform inner SHA256 */
	_addr[0] = k_pad;
	_len[0] = 64;
	for (i = 0; i < num_elem; i++) {
		_addr[i + 1] = addr[i];
		_len[i + 1] = len[i];
	}

#if 0
	for (i=0; i<(1+num_elem); i++) {
		cli_printf("index: %d\r\n", i);
		cli_hexdump(_addr[i], _len[i]);
	}
#endif


#if USE_HARD_HASH
		if (drv_sha256_vector(1 + num_elem, _addr, (unsigned int *)_len, mac) < 0)
			return -1;

#if USE_HARD_HASH_CHECK
		sha256_vector(1 + num_elem, _addr, _len, macdata);
		if (memcmp(mac, macdata, 32)) {
			for (i=0; i<(1+num_elem); i++) {
				cli_printf("index: %d\r\n", i);
				cli_hexdump(_addr[i], _len[i]);
			}
			cli_printf("hash: hard %d\r\n", hash_cnt);
			cli_hexdump(mac, 32);

			cli_printf("hash: soft %d\r\n", hash_cnt);
			cli_hexdump(macdata, 32);
		}

		hash_cnt++;
#endif
	
#else
		if (sha256_vector(1 + num_elem, _addr, _len, mac) < 0)
			return -1;
#endif


#if 0
	cli_printf("hash: soft\r\n");
	cli_hexdump(mac, 32);

	drv_sha256_vector(1 + num_elem, _addr, _len, macdata);

	cli_printf("hash: hard\r\n");
	cli_hexdump(macdata, 32);
#endif

	os_memset(k_pad, 0, sizeof(k_pad));
	os_memcpy(k_pad, key, key_len);
	/* XOR key with opad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x5c;

	/* perform outer SHA256 */
	_addr[0] = k_pad;
	_len[0] = 64;
	_addr[1] = mac;
	_len[1] = SHA256_MAC_LEN;


#if USE_HARD_HASH

#if USE_HARD_HASH_CHECK
	os_memcpy(tk, mac, 32);
	_addr[1] = tk;
#endif

	drv_sha256_vector(2, _addr, (unsigned int *)_len, mac);

#if USE_HARD_HASH_CHECK
	sha256_vector(2, _addr, _len, macdata);
	if (memcmp(mac, macdata, 32)) {
		for (i=0; i<2; i++) {
			cli_printf("index: %d\r\n", i);
			cli_hexdump(_addr[i], _len[i]);
		}
		cli_printf("hash: hard\r\n");
		cli_hexdump(mac, 32);

		cli_printf("hash: soft\r\n");
		cli_hexdump(macdata, 32);
	}
#endif

	return 0;

#else
	return sha256_vector(2, _addr, _len, mac);
#endif
}


/**
 * hmac_sha256 - HMAC-SHA256 over data buffer (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @data: Pointers to the data area
 * @data_len: Length of the data area
 * @mac: Buffer for the hash (32 bytes)
 * Returns: 0 on success, -1 on failure
 */
int hmac_sha256(const u8 *key, size_t key_len, const u8 *data,
		size_t data_len, u8 *mac)
{
	return hmac_sha256_vector(key, key_len, 1, &data, &data_len, mac);
}
