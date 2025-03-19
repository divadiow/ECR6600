/**
 ****************************************************************************************
 *
 * @file net_al.h
 *
 * @brief Declaration of the networking stack abstraction layer.
 * The functions declared here shall be implemented in the networking stack and call the
 * corresponding functions.
 *
 * Copyright (C) RivieraWaves 2017-2018
 *
 ****************************************************************************************
 */

#ifndef NET_AL_H_
#define NET_AL_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "net_def.h"

// Forward declarations
struct fhost_vif_tag;

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
 * @brief Add a network interface
 *
 * @param[in] net_if Pointer to the net_if structure to add
 * @param[in] ipaddr Pointer to the IP address of the interface (NULL if not available)
 * @param[in] netmask Pointer to the net mask of the interface (NULL if not available)
 * @param[in] gw Pointer to the gateway address of the interface (NULL if not available)
 * @param[in] vif Pointer to the VIF information structure associated to this interface
 *
 * @return 0 on success and != 0 if error occurred
 ****************************************************************************************
 */
int net_if_add(net_if_t *net_if,
               const uint32_t *ipaddr,
               const uint32_t *netmask,
               const uint32_t *gw,
               void *state);

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
 * @brief Call the networking stack input function.
 * This function is supposed to link the payload data and length to the RX buffer
 * structure passed as parameter. The free_fn function shall be called when the networking
 * stack is not using the buffer anymore.
 *
 * @param[in] buf Pointer to the RX buffer structure
 * @param[in] net_if Pointer to the net_if structure that receives the packet
 * @param[in] addr Pointer to the payload data
 * @param[in] len Length of the data available at payload address
 * @param[in] free_fn Pointer to buffer freeing function to be called after use
 *
 * @return 0 on success and != 0 if packet is not accepted
 ****************************************************************************************
 */
int net_if_input(net_buf_rx_t *buf, net_if_t *net_if, void *addr, uint16_t len, net_buf_free_fn free_fn);

/**
 ****************************************************************************************
 * @brief Get the pointer to the VIF structure attached to a net interface.
 *
 * @param[in] net_if Pointer to the net_if structure
 *
 * @return The pointer to the VIF structure attached to a net interface
 ****************************************************************************************
 */
struct fhost_vif_tag *net_if_vif_info(net_if_t *net_if);

/**
 ****************************************************************************************
 * @brief Fills the descriptor structure of a buffer.
 * The descriptor structure is stored in the headroom of the buffer. The payload addresses
 * and lengths are filled.
 * The function also returns the pointer to the descriptor space.
 *
 * @param[in]  buf Pointer to the TX buffer structure
 *
 * @return The pointer to the descriptor space of the buffer
 ****************************************************************************************
 */
struct fhost_tx_desc_tag *net_buf_tx_info(net_buf_tx_t **buf);

/**
 ****************************************************************************************
 * @brief Free a TX buffer that was involved in a transmission.
 *
 * @param[in] buf Pointer to the TX buffer structure
 ****************************************************************************************
 */
void net_buf_tx_free(net_buf_tx_t *buf);

/**
 ****************************************************************************************
 * @brief Initialize the networking stack
 *
 * @return 0 on success and != 0 if packet is not accepted
 ****************************************************************************************
 */
int net_init(void);

/**
 ****************************************************************************************
 * @brief Start a TCP iperf server on the given port.
 * The server can then be stopped by calling @ref net_iperf_server_stop with the handle
 * returned by this function as parameter.
 *
 * @return The handle to the created server, NULL in case of failure
 ****************************************************************************************
 */
void *net_iperf_server_start(uint16_t local_port);

/**
 ****************************************************************************************
 * @brief Stop a TCP iperf server previously started with @ref net_iperf_server_start.
 *
 * @param[in] The handle of the server, as returned by @ref net_iperf_server_start
 ****************************************************************************************
 */
void net_iperf_server_stop(void *handle);

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
int net_l2_send(net_if_t *net_if, const uint8_t *data, int data_len, uint16_t ethertype,
                const void *dst_addr);

/**
 ****************************************************************************************
 * @brief Create a L2 (aka ethernet) socket for specific packet
 *
 * Create a L2 socket that will receive specified frames: given ethertype on a given
 * interface.
 * It is expected to fail if a L2 socket for the same ethertype/interface couple already
 * exists.
 *
 * Note: As L2 sockets are not specified in POSIX standard, the implementation of such
 * function may be impossible in some network stack.
 *
 * @param[in] net_if    Pointer to the net_if structure.
 * @param[in] ethertype Ethernet type to filter. (in host endianess)
 *
 * @return <0 if error occurred and the socket descriptor otherwise.
 ****************************************************************************************
 */
int net_l2_socket_create(net_if_t *net_if, uint16_t ethertype);

/**
 ****************************************************************************************
 * @brief Delete a L2 (aka ethernet) socket
 *
 * @param[in] sock Socket descriptor returned by @ref net_l2_socket_create
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 **/
int net_l2_socket_delete(int sock);


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

int net_packet_send(int vif_idx, const uint8_t *data, int data_len, int raw_flag);


#endif // NET_AL_H_
