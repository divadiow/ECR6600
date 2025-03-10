#ifndef __VNET_FILTER_H__
#define __VNET_FILTER_H__
#include "lwip/sockets.h"

/* packet sendto local lwip */
#define VNET_PACKET_DICTION_LWIP 0x0
/* packet sendto host lwip */
#define VNET_PACKET_DICTION_HOST 0x1
/* packet sendto both local/host lwip */
#define VNET_PACKET_DICTION_BOTH 0x2

#define VNET_FILTER_TYPE_IPV4 0x1

typedef enum
{
    VNET_FILTER_MASK_IPADDR          = 0x01,
    VNET_FILTER_MASK_PROTOCOL        = 0x02,
    VNET_FILTER_MASK_DST_PORT        = 0x04,
    VNET_FILTER_MASK_DST_PORT_RANGE  = 0x08,
    VNET_FILTER_MASK_SRC_PORT        = 0x10,
    VNET_FILTER_MASK_SRC_PORT_RANGE  = 0x20,
    VNET_FILTER_MASK_MAX
} VNET_FILTER_MASK_E;

typedef struct
{
    unsigned int ipaddr;
    unsigned short dstPort;
    unsigned short dstPmin;
    unsigned short dstPmax;
    unsigned short srcPort;
    unsigned short srcPmin;
    unsigned short srcPmax;
    unsigned char packeType;
    unsigned char dir;
    unsigned char mask;
    unsigned char resv;
} vnet_ipv4_filter;

void vnet_dump_filter();
int vnet_get_default_filter(void);
int vnet_set_default_filter(unsigned char dir);
int vnet_add_filter(void *filter, unsigned int len, unsigned char ip_type);
int vnet_del_filter(void *filter, unsigned int len, unsigned char ip_type);
int vnet_query_filter(void **filter, unsigned int *len, unsigned char ip_type);
int vnet_ipv4_packet_list_filter(unsigned char *packet);
int vnet_ipv4_packet_blist_filter(unsigned char *packet);
int vnet_filter_wifi_handle(uint16_t type, unsigned char *data);

#endif
