#ifndef __WIFI_COMMON_H__
#define __WIFI_COMMON_H__

#define FHOST_VIF_NUM_MAX 2

#define MAC_ADDR_LENGTH 6

typedef struct
{
    /// Array of 16-bit words that make up the MAC address.
    uint16_t array[MAC_ADDR_LENGTH/2];
} mac_addr_t;

/// Structure of Ethernet Header
struct mac_eth_hdr
{
    /// Destination Address
    mac_addr_t da;
    /// Source Address
    mac_addr_t sa;
    /// Length / Type
    uint16_t len;
} __attribute__ ((__packed__));
typedef struct netif        net_if_t;
extern net_if_t *fhost_get_netif_by_idx(uint8_t idx);
extern net_if_t *fhost_get_netif_by_idx(uint8_t idx);
extern uint8_t fhost_get_idx_by_netif(net_if_t *net_if);
#endif