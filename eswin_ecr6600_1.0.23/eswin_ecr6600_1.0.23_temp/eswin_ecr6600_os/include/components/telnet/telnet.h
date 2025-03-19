#ifndef __TELNET_H__
#define __TLENET_H__
#include "stdarg.h"

/* Telnet protocol stuff ****************************************************/
#define TELNET_NL             0x0a
#define TELNET_CR             0x0d

/* Telnet commands */
#define TELNET_ECHO           1
#define TELNET_SGA            3     /* Suppress Go Ahead */
#define TELNET_NAWS           31    /* Negotiate about window size */

/* Telnet control */
#define TELNET_IAC            255
#define TELNET_WILL           251
#define TELNET_WONT           252
#define TELNET_DO             253
#define TELNET_DONT           254
#define TELNET_SB             250
#define TELNET_SE             240



/* The state of the telnet parser */
enum telnet_state_e
{
	STATE_NORMAL = 0,
	STATE_IAC,
	STATE_WILL,
	STATE_WONT,
	STATE_DO,
	STATE_DONT,
	STATE_SB,
	STATE_SB_NAWS,
	STATE_SE
};


struct telnet_RingBuffer
{
	char *buffer;
	int  buffer_len;
	int  read_pos;
	int  write_pos;
};

#if defined(CONFIG_TELNET)

int telnet_vprintf(const char *f,  va_list ap);

int telnet_log_write(char * log, int len);

int telnet_init(void);


#else
#define telnet_vprintf(f,ap) (-1)

#define telnet_log_write(log,len) (-1)
#endif
#endif //__TLENET_H__
