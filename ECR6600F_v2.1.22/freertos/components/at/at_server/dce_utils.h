#ifndef __DCE_UTILS
#define __DCE_UTILS

#include "stddef.h"
#include "stdint.h"
#include "string.h"

int dce_ishex(char c);

char dce_htoi(char c);

int dce_expect_number(const char** buf, size_t *psize, int def_val);
int dce_expect_string(char** pbuf, size_t* psize, char** result);
int dce_expect_string_no_mark(char** pbuf, size_t* psize, char** result);
void dce_itoa(int val, char* buf, size_t bufsize, size_t* outsize);
void dce_itoa_zeropad(int val, char* buf, size_t bufsize);  // works for nonnegative numbers
void dce_strcpy(const char* str, char* buf, size_t bufsize, size_t* outsize);
int dce_parse_ip(const char* buf, uint8_t* ip); // returns 0 on success
#endif //__DCE_UTILS
