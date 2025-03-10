#ifndef __BASIC_CMD
#define __BASIC_CMD

#include "stddef.h"
#include "stdint.h"
#include "string.h"

void dce_register_basic_commands(dce_t* dce);
uint8_t get_system_config(void);

#endif