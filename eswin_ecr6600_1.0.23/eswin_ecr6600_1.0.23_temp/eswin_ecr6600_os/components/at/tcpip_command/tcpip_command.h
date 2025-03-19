#ifndef __TCPIP_CMD
#define __TCPIP_CMD

#include "stddef.h"
#include "stdint.h"
#include "string.h"

#define AT_CHECK_ERROR_RETURN(x)  do {\
                                        if (x) {\
                                            dce_emit_basic_result_code(dce, DCE_RC_ERROR); \
                                            return DCE_RC_ERROR; }\
                                  } while(0)
                                            

void dce_register_tcpip_commands(dce_t* dce);
void hexStr2MACAddr(uint8_t *hexStr, uint8_t* mac );
#endif
