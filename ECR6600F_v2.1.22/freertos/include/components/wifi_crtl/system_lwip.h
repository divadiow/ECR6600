/*******************************************************************************
 * Copyright by ESWIN.
 *
 * File Name:    system_lwip.h
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       V1.0
 * Author:        baoyong
 * Date:          2022-03-12
 * History 1:      
 *     Date: 
 *     Version:
 *     Author: 
 *     Modification:  
 * History 2: 
  ********************************************************************************/

#ifndef _SYSTEM_LWIP_H
#define _SYSTEM_LWIP_H

/****************************************************************************
* 	                                        Include files
****************************************************************************/
#include "net_def.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// Prototype for a function to free a network buffer */
typedef void (*net_buf_free_fn)(void *net_buf);

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Call the checksum computation function of the TCP/IP stack
 *
 * @param[in] dataptr Pointer to the data buffer on which the checksum is computed
 * @param[in] len Length of the data buffer
 *
 * @return The computed checksum
 ****************************************************************************************
 */
uint16_t net_ip_chksum(const void *dataptr, int len);

/**
 ****************************************************************************************
 * @brief Get network interface MAC address
 *
 * @param[in] net_if Pointer to the net_if structure
 *
 * @return Pointer to interface MAC address
 ****************************************************************************************
 */
const uint8_t *net_if_get_mac_addr(net_if_t *net_if);

/**
 ****************************************************************************************
 * @brief Get pointer to network interface from its name
 *
 * @param[in] name Name of the interface
 *
 * @return pointer to the net_if structure and NULL if such interface doesn't exist.
 ****************************************************************************************
 */
net_if_t *net_if_find_from_name(const char *name);

/**
 ****************************************************************************************
 * @brief Get pointer to network interface from its wifi index
 *
 * @param[in] idx Index of the wifi interface
 *
 * @return pointer to the net_if structure and NULL if such interface doesn't exist.
 ****************************************************************************************
 */
//net_if_t *net_if_find_from_wifi_idx(unsigned int idx);

/**
 ****************************************************************************************
 * @brief Get name of network interface
 *
 * Passing a buffer of at least @ref NET_AL_MAX_IFNAME bytes, will ensure that it is big
 * enough to contain the interface name.
 *
 * @param[in]     net_if Pointer to the net_if structure
 * @param[in]     buf    Buffer to write the interface name
 * @param[in,out] len    Length of the buffer, updated with the actual length of the
 *                       interface name (not including terminating null byte)
 *
 * @return 0 on success and != 0 if error occurred
 ****************************************************************************************
 */
int net_if_get_name(net_if_t *net_if, char *buf, int *len);

/**
 ****************************************************************************************
 * @brief Get index of the wifi interface for a network interface
 *
 * @param[in] net_if Pointer to the net_if structure
 *
 * @return <0 if cannot find the interface and the wifi interface index otherwise
 ****************************************************************************************
 */
//int net_if_get_wifi_idx(net_if_t *net_if);

/**
 ****************************************************************************************
 * @brief Indicate that the network interface is now up (i.e. able to do traffic)
 *
 * @param[in] net_if Pointer to the net_if structure
 ****************************************************************************************
 */
void net_if_up(net_if_t *net_if);

/**
 ****************************************************************************************
 * @brief Indicate that the network interface is now down
 *
 * @param[in] net_if Pointer to the net_if structure
 ****************************************************************************************
 */
void net_if_down(net_if_t *net_if);

/**
 ****************************************************************************************
 * @brief Set IPv4 address of an interface
 *
 * It is assumed that only one address can be configured on the interface and then
 * setting a new address can be used to replace/delete the current address.
 *
 * @param[in] net_if Pointer to the net_if structure
 * @param[in] ip     IPv4 address
 * @param[in] mask   IPv4 network mask
 * @param[in] gw     IPv4 gateway address
 ****************************************************************************************
 */
void net_if_set_ip(net_if_t *net_if, uint32_t ip, uint32_t mask, uint32_t gw);

/**
 ****************************************************************************************
 * @brief Get IPv4 address of an interface
 *
 * Set to NULL parameter you're not interested in.
 *
 * @param[in]  net_if Pointer to the net_if structure
 * @param[out] ip     IPv4 address
 * @param[out] mask   IPv4 network mask
 * @param[out] gw     IPv4 gateway address
 * @return 0 if requested parameters have been updated successfully and !=0 otherwise.
 ****************************************************************************************
 */
int net_if_get_ip(net_if_t *net_if, uint32_t *ip, uint32_t *mask, uint32_t *gw);

/**
 ****************************************************************************************
 * @brief Free a TX buffer that was involved in a transmission.
 *
 * @param[in] buf Pointer to the TX buffer structure
 ****************************************************************************************
 */
void net_tx_buf_free(void *buf);

/**
 ****************************************************************************************
 * @brief Send a L2 (aka ethernet) packet
 *
 * Send data on the link layer (L2). If destination address is not NULL, Ethernet header
 * will be added (using ethertype parameter) and MAC address of the sending interface is
 * used as source address. If destination address is NULL, it means that ethernet header
 * is already present and frame should be send as is.
 * The data buffer will be copied by this function, and must then be freed by the caller.
 *
 * The primary purpose of this function is to allow the supplicant sending EAPOL frames.
 * As these frames are often followed by addition/deletion of crypto keys, that
 * can cause encryption to be enabled/disabled in the MAC, it is required to ensure that
 * the packet transmission is completed before proceeding to the key setting.
 * This function shall therefore be blocking until the frame has been transmitted by the
 * MAC.
 *
 * @param[in] net_if    Pointer to the net_if structure.
 * @param[in] data      Data buffer to send.
 * @param[in] data_len  Buffer size, in bytes.
 * @param[in] ethertype Ethernet type to set in the ethernet header. (in host endianess)
 * @param[in] dst_addr  Ethernet address of the destination. If NULL then it means that
 *                      ethernet header is already present in the frame (and in this case
 *                      ethertype should be ignored)
 *
 * @return 0 on success and != 0 if packet hasn't been sent
 ****************************************************************************************
 */
int net_l2_send(void *net_if, const uint8_t *data, int data_len, uint16_t ethertype,
                const void *dst_addr);

/**
 ****************************************************************************************
 * @brief send 802.11 raw data
 *
 * @param[in] vif_idx   idx Index of the wifi interface default 0
 * @param[in] data      Data buffer to send.(include 802.11 header)
 * @param[in] data_len  Buffer size, in bytes.
 * @param[in] raw_flag  send without any change.
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
**/

int net_packet_send(int vif_idx, const uint8_t *data, int data_len, uint8_t is_raw, uint8_t is_block);

/**
 ****************************************************************************************
 * @brief upload the rxbuf to network stack
 *
 * @param[in] buf the point to the netbuf
 * @param[in] net_if the porint to the netif 
 * @param[in] addr the point to the addr of the payload 
 * @param[in] len the len of the payload
 * @param[in] free_fn the func to free the netbuf
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
**/
int wifi_net_input(void *buf, net_if_t *net_if, void *addr, uint16_t len, net_buf_free_fn free_fn);

/**
 ****************************************************************************************
 * @brief do the  net init  for wifi
 *
 * @return void 
 ****************************************************************************************
**/
void wifi_net_init(void);
/**
 ****************************************************************************************
 * @brief get the encapsulation header len for the tx buf, which is used privacily and is 
 * reserved in the head of the txbuf.
 *
 * @return the len of the headroom.
 ****************************************************************************************
 */

int16_t net_tx_buf_encapsulation_header_len(void);
/**
 ****************************************************************************************
 * @brief reserve the headerlen of the txbuf, and return the new payload ptr of the txbuf
 * @param[in] buf Pointer to the txbuf
 *
 * @return the new payload ptr of the txbuf
 ****************************************************************************************
 */
void *net_tx_buf_get_encapsulation_header(void *buf);
/**
 ****************************************************************************************
 * @brief get the total lens of the buflist
 * @param[in] buf Pointer to the txbuf
 *
 * @return the total lens of the buflist
 ****************************************************************************************
 */
uint16_t net_tx_buf_get_total_len(void *buf);
/**
 ****************************************************************************************
 * @brief get the ptr of the payload of the txbuf
 * @param[in] buf Pointer to the txbuf
 *
 * @return the ptr of the payload of the txbuf
 ****************************************************************************************
 */
void *net_tx_buf_get_payload(void *buf);
/**
 ****************************************************************************************
 * @brief get the lens of the current txbuf
 * @param[in] buf Pointer to the txbuf
 *
 * @return the lens of the current txbuf
 ****************************************************************************************
 */
uint16_t net_tx_buf_get_len(void *buf);
/**
 ****************************************************************************************
 * @brief get the next txbuf of the current txbuf
 * @param[in] buf Pointer to the txbuf
 *
 * @return the next txbuf of the current txbuf
 ****************************************************************************************
 */
void *net_tx_buf_get_next(void *buf);

/**
 ****************************************************************************************
 * @brief alloc a block of memory with the assigned size
 * @param[in] len the size of the memory
 *
 * @return the point of the memory
 ****************************************************************************************
 */
void *net_tx_mem_alloc(uint32_t len);
/**
 ****************************************************************************************
 * @brief free the assigned  memory 
 * @param[in] mem the point of the memory
 *
 * @return void 
 ****************************************************************************************
 */
void net_tx_mem_free(void *mem);

/**
 ****************************************************************************************
 * @brief get the extra size of the rxbuf, which is alloced with the rxbuf together, and 
 * is used to pass to the network stack.
 *
 * @return  the extra size of the rxbuf
 ****************************************************************************************
 */
uint16_t net_rx_buf_extra_size(void);

#if defined ENABLE_LWIP_NAPT || (CONFIG_SPI_REPEATER && CONFIG_SPI_MASTER)
int enable_lwip_napt(int vif, int enable);
#endif

#endif/*_SYSTEM_LWIP_H*/

