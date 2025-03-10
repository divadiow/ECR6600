#ifndef __VNET_REG__H__
#define __VNET_REG__H__

#define VNET_REG_ATTRI_RO  1
#define VNET_REG_ATTRI_WO  2
#define VNET_REG_ATTRI_RW  3

#define VNET_WIFI_LINK_DOWN 0
#define VNET_WIFI_LINK_UP   1

#define VNET_REG_DEBUG_SUPPORT

typedef struct
{
   volatile unsigned int status;
   volatile unsigned int intFlag;
   volatile unsigned int intMask;
   volatile unsigned int intClr;
   volatile unsigned int ipv4Addr;
   volatile unsigned int ipv4Mask;
   volatile unsigned int macAddr0;
   volatile unsigned int macAddr1;
   volatile unsigned int gw0;
   volatile unsigned int gw1;
   volatile unsigned int gw2;
   volatile unsigned int gw3;
   volatile unsigned int dns0;
   volatile unsigned int dns1;
   volatile unsigned int dns2;
   volatile unsigned int dns3;
   volatile unsigned int fwVersion;
   volatile unsigned int powerOff;
}vnet_reg_t;

typedef void (*vreg_call)(void);
typedef struct
{
    unsigned int offset;
    unsigned int regAddr;
    unsigned int attri;
    vreg_call opCall;
} vnet_reg_op_t;

vnet_reg_op_t *vnet_reg_get_table(unsigned int offset);
void vnet_reg_wifilink_status(int link);
void vnet_reg_op_call(unsigned int addr);
vnet_reg_t *vnet_reg_get_addr(void);
void vnet_reg_get_info(void);
void vnet_reg_get_peer_value(void);

#endif
