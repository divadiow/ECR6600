/**
 ****************************************************************************************
 *
 * @file net_tg_al.h
 *
 * @brief Declaration of the networking stack abstraction layer used for traffic generator.
 * The functions declared here shall be implemented in the networking stack and call the
 * corresponding functions.
 *
 * Copyright (C) RivieraWaves 2017-2018
 *
 ****************************************************************************************
 */

#ifndef NET_TG_AL_H_
#define NET_TG_AL_H_

/*
 * DEFINITIONS
 ****************************************************************************************
 */
// Forward declarations
struct fhost_tg_stream;

/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Send UDP packet to a destination IP address without allocating memory for buffer
 * This function will send datagram to specific IP address and port.
 * The packet to send is divided into two parts :
 *      - TG header : it includes header message, counter, and time stamp
 *      - TG buffer : it contains a pointer which is pointed to a static memory zone
 * These two parts are chained together as a whole packet.
 * The interval time of sending is limited to 20 ms refering to net_tg_sleep_time()
 *
 * @param[in] my_stream    pointer to TG Stream
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 **/
int net_tg_send(struct fhost_tg_stream *my_stream);

/**
 ****************************************************************************************
 * @brief Start receiving UDP packet from any sender
 * This function will bind the local port of the server and configure the callback
 * function which will be called every time there's datagram from the configured PCB
 *
 * @param[in] my_stream    pointer to TG Stream
 *
 * @return 0 on success and != 0 if error occurred.
 ****************************************************************************************
 **/
int net_tg_recv_start(struct fhost_tg_stream *my_stream);

/**
 ****************************************************************************************
 * @brief Stop receiving UDP packet from any sender
 * This function will free the UDP PCB of the receiving callback function so that it will
 * not be called.
 *
 * @param[in] my_stream    pointer to TG Stream
 ****************************************************************************************
 **/
void net_tg_recv_stop(struct fhost_tg_stream *my_stream);

#endif // NET_TG_AL_H_
