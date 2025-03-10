/**
 * @file
 *
 * @defgroup dhcp6 DHCPv6
 * @ingroup ip6
 * DHCPv6 client: IPv6 address autoconfiguration as per
 * RFC 3315 (stateful DHCPv6) and
 * RFC 3736 (stateless DHCPv6).
 *
 * For now, only stateless DHCPv6 is implemented!
 *
 * TODO:
 * - enable/disable API to not always start when RA is received
 * - stateful DHCPv6 (for now, only stateless DHCPv6 for DNS and NTP servers works)
 * - create Client Identifier?
 * - only start requests if a valid local address is available on the netif
 * - only start information requests if required (not for every RA)
 *
 * dhcp6_enable_stateful() enables stateful DHCPv6 for a netif (stateless disabled)\n
 * dhcp6_enable_stateless() enables stateless DHCPv6 for a netif (stateful disabled)\n
 * dhcp6_disable() disable DHCPv6 for a netif
 *
 * When enabled, requests are only issued after receipt of RA with the
 * corresponding bits set.
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 */

#include "lwip/opt.h"

#if LWIP_IPV6 && LWIP_IPV6_DHCP6 /* don't build if not configured for use in lwipopts.h */

#include "lwip/dhcp6.h"
#include "lwip/prot/dhcp6.h"
#include "lwip/prot/ethernet.h"
#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

#include <string.h>

#ifdef LWIP_HOOK_FILENAME
#include LWIP_HOOK_FILENAME
#endif
#ifndef LWIP_HOOK_DHCP6_APPEND_OPTIONS
#define LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, state, msg, msg_type, options_len_ptr, max_len)
#endif
#ifndef LWIP_HOOK_DHCP6_PARSE_OPTION
#define LWIP_HOOK_DHCP6_PARSE_OPTION(netif, dhcp6, state, msg, msg_type, option, len, pbuf, offset) do { LWIP_UNUSED_ARG(msg); } while(0)
#endif

#if LWIP_DNS && LWIP_DHCP6_MAX_DNS_SERVERS
#if DNS_MAX_SERVERS > LWIP_DHCP6_MAX_DNS_SERVERS
#define LWIP_DHCP6_PROVIDE_DNS_SERVERS LWIP_DHCP6_MAX_DNS_SERVERS
#else
#define LWIP_DHCP6_PROVIDE_DNS_SERVERS DNS_MAX_SERVERS
#endif
#else
#define LWIP_DHCP6_PROVIDE_DNS_SERVERS 0
#endif

#define DHCPV6_TIMER_SOL_TIMEOUT      (1)
#define DHCPV6_TIMER_SOL_MAX_RT       (3600)
#define DHCPV6_TIMER_REQ_TIMEOUT      (1)
#define DHCPV6_TIMER_REQ_MAX_RT       (30)
#define DHCPV6_TIMER_REQ_MAX_RC       (10)
#define DHCPV6_TIMER_REN_TIMEOUT      (10)
#define DHCPV6_TIMER_REN_MAX_RT       (600)
#define DHCPV6_TIMER_REB_TIMEOUT      (10)
#define DHCPV6_TIMER_REB_MAX_RT       (600)
#define DHCPV6_TIMER_REL_TIMEOUT      (1)
#define DHCPV6_TIMER_REL_MAX_RC       (4)
#define DHCPV6_TIMER_DEC_TIMEOUT      (1)
#define DHCPV6_TIMER_DEC_MAX_RC       (4)

#define DHCPV6_SERVER_ID_LEN_MAX      (64)
#define DHCPV6_RETRY_MAX              (25)
#define DHCPV6_IA_ID_IA_NA            (1)

#define DHCPV6_OPT_HEAD_LEN           (4)
#define DHCPV6_IANA_WITH_ADDR_LEN     (40)
#define DHCPV6_IANA_NO_ADDR_LEN       (12)
#define DHCPV6_IA_ADDR_LEN            (24)
#define DHCPV6_CLIENT_ID_LEN          (10)
#define DHCPV6_ELAPSED_TIME_LEN       (2)
#define DHCPV6_STATUS_CODE_LEN        (2)
#define DHCPV6_IA_ID_LEN              (4)
#define DHCPV6_T1_LEN                 (4)
#define DHCPV6_T2_LEN                 (4)
#define DHCPV6_PERFER_TIMER_LEN       (4)
#define DHCPV6_VALID_TIMER_LEN        (4)
#define DHCPV6_IANA_WITH_ADDR_STATUS_LEN     (46)		/*兼容NETGEAR_62路由器,[ECR6600-1212]*/

/** Option handling: options are parsed in dhcp6_parse_reply
 * and saved in an array where other functions can load them from.
 * This might be moved into the struct dhcp6 (not necessarily since
 * lwIP is single-threaded and the array is only used while in recv
 * callback). */
enum dhcp6_option_idx {
  DHCP6_OPTION_IDX_CLI_ID = 0,
  DHCP6_OPTION_IDX_SERVER_ID,
  DHCP6_OPTION_IDX_STATUS,
  DHCP6_OPTION_IDX_IA_NA,
#if LWIP_DHCP6_PROVIDE_DNS_SERVERS
  DHCP6_OPTION_IDX_DNS_SERVER,
  DHCP6_OPTION_IDX_DOMAIN_LIST,
#endif /* LWIP_DHCP_PROVIDE_DNS_SERVERS */
#if LWIP_DHCP6_GET_NTP_SRV
  DHCP6_OPTION_IDX_NTP_SERVER,
#endif /* LWIP_DHCP_GET_NTP_SRV */
  DHCP6_OPTION_IDX_MAX
};

struct dhcp6_option_info {
  u8_t option_given;
  u16_t val_start;
  u16_t val_length;
};

/** Holds the decoded option info, only valid while in dhcp6_recv. */
struct dhcp6_option_info dhcp6_rx_options[DHCP6_OPTION_IDX_MAX];

#define dhcp6_option_given(dhcp6, idx)           (dhcp6_rx_options[idx].option_given != 0)
#define dhcp6_got_option(dhcp6, idx)             (dhcp6_rx_options[idx].option_given = 1)
#define dhcp6_clear_option(dhcp6, idx)           (dhcp6_rx_options[idx].option_given = 0)
#define dhcp6_clear_all_options(dhcp6)           (memset(dhcp6_rx_options, 0, sizeof(dhcp6_rx_options)))
#define dhcp6_get_option_start(dhcp6, idx)       (dhcp6_rx_options[idx].val_start)
#define dhcp6_get_option_length(dhcp6, idx)      (dhcp6_rx_options[idx].val_length)
#define dhcp6_set_option(dhcp6, idx, start, len) do { dhcp6_rx_options[idx].val_start = (start); dhcp6_rx_options[idx].val_length = (len); }while(0)


const ip_addr_t dhcp6_All_DHCP6_Relay_Agents_and_Servers = IPADDR6_INIT_HOST(0xFF020000, 0, 0, 0x00010002);
const ip_addr_t dhcp6_All_DHCP6_Servers = IPADDR6_INIT_HOST(0xFF020000, 0, 0, 0x00010003);

static struct udp_pcb *dhcp6_pcb;
static u8_t dhcp6_pcb_refcount;


/* receive, unfold, parse and free incoming messages */
static void dhcp6_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
#if LWIP_IPV6_DHCP6_STATEFUL
static void dhcp6_stateful_solicit(struct netif *netif, struct dhcp6 *dhcp6);
#endif
/** Ensure DHCP PCB is allocated and bound */
static err_t
dhcp6_inc_pcb_refcount(void)
{
  if (dhcp6_pcb_refcount == 0) {
    LWIP_ASSERT("dhcp6_inc_pcb_refcount(): memory leak", dhcp6_pcb == NULL);

    /* allocate UDP PCB */
    dhcp6_pcb = udp_new_ip6();

    if (dhcp6_pcb == NULL) {
      return ERR_MEM;
    }

    ip_set_option(dhcp6_pcb, SOF_BROADCAST);

    /* set up local and remote port for the pcb -> listen on all interfaces on all src/dest IPs */
    udp_bind(dhcp6_pcb, IP6_ADDR_ANY, DHCP6_CLIENT_PORT);
    udp_recv(dhcp6_pcb, dhcp6_recv, NULL);
  }

  dhcp6_pcb_refcount++;

  return ERR_OK;
}

/** Free DHCP PCB if the last netif stops using it */
static void
dhcp6_dec_pcb_refcount(void)
{
  LWIP_ASSERT("dhcp6_pcb_refcount(): refcount error", (dhcp6_pcb_refcount > 0));
  dhcp6_pcb_refcount--;

  if (dhcp6_pcb_refcount == 0) {
    udp_remove(dhcp6_pcb);
    dhcp6_pcb = NULL;
  }
}

/**
 * @ingroup dhcp6
 * Set a statically allocated struct dhcp6 to work with.
 * Using this prevents dhcp6_start to allocate it using mem_malloc.
 *
 * @param netif the netif for which to set the struct dhcp
 * @param dhcp6 (uninitialised) dhcp6 struct allocated by the application
 */
void
dhcp6_set_struct(struct netif *netif, struct dhcp6 *dhcp6)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("dhcp6 != NULL", dhcp6 != NULL);
  LWIP_ASSERT("netif already has a struct dhcp6 set", netif_dhcp6_data(netif) == NULL);

  /* clear data structure */
  memset(dhcp6, 0, sizeof(struct dhcp6));
  /* dhcp6_set_state(&dhcp, DHCP6_STATE_OFF); */
  netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP6, dhcp6);
}

/**
 * @ingroup dhcp6
 * Removes a struct dhcp6 from a netif.
 *
 * ATTENTION: Only use this when not using dhcp6_set_struct() to allocate the
 *            struct dhcp6 since the memory is passed back to the heap.
 *
 * @param netif the netif from which to remove the struct dhcp
 */
void dhcp6_cleanup(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);

  if (netif_dhcp6_data(netif) != NULL) {
    mem_free(netif_dhcp6_data(netif));
    netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP6, NULL);
  }
}

static struct dhcp6*
dhcp6_get_struct(struct netif *netif, const char *dbg_requester)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  if (dhcp6 == NULL) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("%s: mallocing new DHCPv6 client\n", dbg_requester));
    dhcp6 = (struct dhcp6 *)mem_malloc(sizeof(struct dhcp6));
    if (dhcp6 == NULL) {
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("%s: could not allocate dhcp6\n", dbg_requester));
      return NULL;
    }

    /* clear data structure, this implies DHCP6_STATE_OFF */
    memset(dhcp6, 0, sizeof(struct dhcp6));
    dhcp6->addr_idx = 0xFF;
    /* store this dhcp6 client in the netif */
    netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP6, dhcp6);
  } else {
    /* already has DHCP6 client attached */
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("%s: using existing DHCPv6 client\n", dbg_requester));
  }

  if (!dhcp6->pcb_allocated) {
    if (dhcp6_inc_pcb_refcount() != ERR_OK) { /* ensure DHCP6 PCB is allocated */
      mem_free(dhcp6);
      netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP6, NULL);
      return NULL;
    }
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("%s: allocated dhcp6", dbg_requester));
    dhcp6->pcb_allocated = 1;
  }
  return dhcp6;
}

/*
 * Set the DHCPv6 state
 * If the state changed, reset the number of tries.
 */
static void
dhcp6_set_state(struct dhcp6 *dhcp6, u8_t new_state, const char *dbg_caller)
{
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("DHCPv6 state: %d -> %d (%s)\n",
    dhcp6->state, new_state, dbg_caller));
  if (new_state != dhcp6->state) {
    dhcp6->state = new_state;
    dhcp6->tries = 0;
    dhcp6->request_timeout = 0;
    dhcp6->elapsed_time = 0;
  }
}

static int
dhcp6_stateless_enabled(struct dhcp6 *dhcp6)
{
  if ((dhcp6->state == DHCP6_STATE_STATELESS_IDLE) ||
      (dhcp6->state == DHCP6_STATE_REQUESTING_CONFIG)) {
    return 1;
  }
  return 0;
}

static int
dhcp6_stateful_enabled(struct dhcp6 *dhcp6)
{
  if ((dhcp6->state == DHCP6_STATE_OFF) ||
      (dhcp6->state == DHCP6_STATE_STATELESS_IDLE) ||
      (dhcp6->state == DHCP6_STATE_REQUESTING_CONFIG))
  {
    return 0;
  }

  return 1;
}

/**
 * @ingroup dhcp6
 * Enable stateful DHCPv6 on this netif
 * Requests are sent on receipt of an RA message with the
 * ND6_RA_FLAG_MANAGED_ADDR_CONFIG flag set.
 *
 * A struct dhcp6 will be allocated for this netif if not
 * set via @ref dhcp6_set_struct before.
 *
 * @todo: stateful DHCPv6 not supported, yet
 */
err_t
dhcp6_enable_stateful(struct netif *netif)
{
  struct dhcp6 *dhcp6;

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_enable_stateful(netif=%p) %c%c%"U16_F"\n", (void *)netif, netif->name[0], netif->name[1], (u16_t)netif->num));

  dhcp6 = dhcp6_get_struct(netif, "dhcp6_enable_stateful()");
  if (dhcp6 == NULL) {
    return ERR_MEM;
  }

  if (dhcp6_stateless_enabled(dhcp6)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_enable_stateful(): stateless DHCPv6 already enabled"));
    return ERR_USE;
  }

  if (dhcp6_stateful_enabled(dhcp6)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_enable_stateful(): stateful DHCPv6 already enabled"));
    return ERR_OK;
  }
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_enabled(): stateless DHCPv6 enabled\n"));

  dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_SOLICIT, "dhcp6_enable_stateful");
  dhcp6_stateful_solicit(netif, dhcp6);

  return ERR_OK;
}

/**
 * @ingroup dhcp6
 * Enable stateless DHCPv6 on this netif
 * Requests are sent on receipt of an RA message with the
 * ND6_RA_FLAG_OTHER_CONFIG flag set.
 *
 * A struct dhcp6 will be allocated for this netif if not
 * set via @ref dhcp6_set_struct before.
 */
err_t
dhcp6_enable_stateless(struct netif *netif)
{
  struct dhcp6 *dhcp6;

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_enable_stateless(netif=%p) %c%c%"U16_F"\n", (void *)netif, netif->name[0], netif->name[1], (u16_t)netif->num));

  dhcp6 = dhcp6_get_struct(netif, "dhcp6_enable_stateless()");
  if (dhcp6 == NULL) {
    return ERR_MEM;
  }
  if (dhcp6_stateful_enabled(dhcp6)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_enable_stateless(): stateful DHCPv6 already enabled"));
    return ERR_USE;
  }
  if (dhcp6_stateless_enabled(dhcp6)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_enable_stateless(): stateless DHCPv6 already enabled"));
    return ERR_OK;
  }
  
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_enable_stateless(): stateless DHCPv6 enabled\n"));
  dhcp6_set_state(dhcp6, DHCP6_STATE_STATELESS_IDLE, "dhcp6_enable_stateless");
  return ERR_OK;
}

/**
 * @ingroup dhcp6
 * Disable stateful or stateless DHCPv6 on this netif
 * Requests are sent on receipt of an RA message with the
 * ND6_RA_FLAG_OTHER_CONFIG flag set.
 */
void
dhcp6_disable(struct netif *netif)
{
  struct dhcp6 *dhcp6;

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_disable(netif=%p) %c%c%"U16_F"\n", (void *)netif, netif->name[0], netif->name[1], (u16_t)netif->num));

  dhcp6 = netif_dhcp6_data(netif);
  if (dhcp6 != NULL) {
    if (dhcp6->state != DHCP6_STATE_OFF) {
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_disable(): DHCPv6 disabled (old state: %s)\n",
        (dhcp6_stateless_enabled(dhcp6) ? "stateless" : "stateful")));
      dhcp6_set_state(dhcp6, DHCP6_STATE_OFF, "dhcp6_disable");
      if (dhcp6->pcb_allocated != 0) {
        dhcp6_dec_pcb_refcount(); /* free DHCPv6 PCB if not needed any more */
        dhcp6->pcb_allocated = 0;
      }
    }
  }
}

/**
 * Create a DHCPv6 request, fill in common headers
 *
 * @param netif the netif under DHCPv6 control
 * @param dhcp6 dhcp6 control struct
 * @param message_type message type of the request
 * @param opt_len_alloc option length to allocate
 * @param options_out_len option length on exit
 * @return a pbuf for the message
 */
static struct pbuf *
dhcp6_create_msg(struct netif *netif, struct dhcp6 *dhcp6, u8_t message_type,
                 u16_t opt_len_alloc, u16_t *options_out_len)
{
  struct pbuf *p_out;
  struct dhcp6_msg *msg_out;

  LWIP_ERROR("dhcp6_create_msg: netif != NULL", (netif != NULL), return NULL;);
  LWIP_ERROR("dhcp6_create_msg: dhcp6 != NULL", (dhcp6 != NULL), return NULL;);
  p_out = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcp6_msg) + opt_len_alloc, PBUF_RAM);
  if (p_out == NULL) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
                ("dhcp6_create_msg(): could not allocate pbuf\n"));
    return NULL;
  }
  LWIP_ASSERT("dhcp6_create_msg: check that first pbuf can hold struct dhcp6_msg",
              (p_out->len >= sizeof(struct dhcp6_msg) + opt_len_alloc));

  /* @todo: limit new xid for certain message types? */
  /* reuse transaction identifier in retransmissions */
  if (dhcp6->tries == 0) {
    dhcp6->xid = LWIP_RAND() & 0xFFFFFF;
  }

  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE,
              ("transaction id xid(%"X32_F")\n", dhcp6->xid));

  msg_out = (struct dhcp6_msg *)p_out->payload;
  memset(msg_out, 0, sizeof(struct dhcp6_msg) + opt_len_alloc);

  msg_out->msgtype = message_type;
  msg_out->transaction_id[0] = (u8_t)(dhcp6->xid >> 16);
  msg_out->transaction_id[1] = (u8_t)(dhcp6->xid >> 8);
  msg_out->transaction_id[2] = (u8_t)dhcp6->xid;
  *options_out_len = 0;
  return p_out;
}

static u16_t
dhcp6_option_short(u16_t options_out_len, u8_t *options, u16_t value)
{
  options[options_out_len++] = (u8_t)((value & 0xff00U) >> 8);
  options[options_out_len++] = (u8_t) (value & 0x00ffU);
  return options_out_len;
}

static u16_t
dhcp6_option_u32(u16_t options_out_len, u8_t *options, u32_t value)
{
  options[options_out_len++] = (u8_t)((value & 0xff000000U) >> 24);
  options[options_out_len++] = (u8_t)((value & 0x00ff0000U) >> 16);
  options[options_out_len++] = (u8_t)((value & 0x0000ff00U) >> 8);
  options[options_out_len++] = (u8_t) (value & 0x000000ffU);
  return options_out_len;
}

static u16_t
dhcp6_option_elapsed_time(u16_t options_out_len, u8_t *options, u16_t elaspsed_time, u16_t max_len)
{
  u16_t options_used;

  options_used = dhcp6_option_short(options_out_len, options, DHCP6_OPTION_ELAPSED_TIME);
  options_used = dhcp6_option_short(options_used, options, DHCPV6_ELAPSED_TIME_LEN);
  options_used = dhcp6_option_short(options_used, options, elaspsed_time*DHCP6_TIMER_MSECS/100);

  LWIP_ASSERT("dhcp6_option_elapsed_time: options_used <= max_len", sizeof(struct dhcp6_msg) + options_used <= max_len);
  LWIP_UNUSED_ARG(max_len);

  return options_used;
}

static u16_t
dhcp6_option_server_id(u16_t options_out_len, u8_t *options, const u8_t *serverID, u16_t ID_len, u16_t max_len)
{
  u16_t options_used;

  options_used = dhcp6_option_short(options_out_len, options, DHCP6_OPTION_SERVERID);
  options_used = dhcp6_option_short(options_used, options, ID_len);
  memcpy(&options[options_used], serverID, ID_len);
  options_used += ID_len;

  LWIP_ASSERT("dhcp6_option_server_id: options_used <= max_len", sizeof(struct dhcp6_msg) + options_used <= max_len);
  LWIP_UNUSED_ARG(max_len);

  return options_used;
}

static u16_t
dhcp6_option_client_id(u16_t options_out_len, u8_t *options, const u8_t *mac, u16_t max_len)
{
  u16_t options_used;

  options_used = dhcp6_option_short(options_out_len, options, DHCP6_OPTION_CLIENTID);
  options_used = dhcp6_option_short(options_used, options, DHCPV6_CLIENT_ID_LEN);
  options_used = dhcp6_option_short(options_used, options, DHCP6_DUID_LL);
  options_used = dhcp6_option_short(options_used, options, DHCP6_HW_TYPE_IEEE802);
  memcpy(&options[options_used], mac, ETH_HWADDR_LEN);
  options_used += ETH_HWADDR_LEN;

  LWIP_ASSERT("dhcp6_option_client_id: options_used <= max_len", sizeof(struct dhcp6_msg) + options_used <= max_len);
  LWIP_UNUSED_ARG(max_len);

  return options_used;
}

static u16_t
dhcp6_option_ia_na(u16_t options_out_len, u8_t *options, struct netif *netif, u16_t option_len, u8_t addr_idx, 
                   u32_t T1, u32_t T2, u32_t prefer, u32_t valid, u16_t max_len)
{
  u16_t options_used;

  options_used = dhcp6_option_short(options_out_len, options, DHCP6_OPTION_IA_NA);
  options_used = dhcp6_option_short(options_used, options, option_len);
  options_used = dhcp6_option_u32(options_used, options, DHCPV6_IA_ID_IA_NA);
  options_used = dhcp6_option_u32(options_used, options, T1);
  options_used = dhcp6_option_u32(options_used, options, T2);

  // IA_NA with IAAddr
  if(option_len == DHCPV6_IANA_WITH_ADDR_LEN)
  {
    ip6_addr_t net_addr;
    options_used = dhcp6_option_short(options_used, options, DHCP6_OPTION_IAADDR);
    options_used = dhcp6_option_short(options_used, options, DHCPV6_IA_ADDR_LEN);
    ip6_addr_set_hton(&net_addr, ip_2_ip6(&netif->ip6_addr[addr_idx]));
    options_used = dhcp6_option_u32(options_used, options, net_addr.addr[0]);
    options_used = dhcp6_option_u32(options_used, options, net_addr.addr[1]);
    options_used = dhcp6_option_u32(options_used, options, net_addr.addr[2]);
    options_used = dhcp6_option_u32(options_used, options, net_addr.addr[3]);
    options_used = dhcp6_option_u32(options_used, options, prefer);
    options_used = dhcp6_option_u32(options_used, options, valid);
  }

  LWIP_ASSERT("dhcp6_option_ia_na: options_used <= max_len", sizeof(struct dhcp6_msg) + options_used <= max_len);
  LWIP_UNUSED_ARG(max_len);

  return options_used;
}

static u16_t
dhcp6_option_optionrequest(u16_t options_out_len, u8_t *options, const u16_t *req_options,
                           u16_t num_req_options, u16_t max_len)
{
  size_t i;
  u16_t ret;

  LWIP_ASSERT("dhcp6_option_optionrequest: options_out_len + sizeof(struct dhcp6_msg) + addlen <= max_len",
    sizeof(struct dhcp6_msg) + options_out_len + 4U + (2U * num_req_options) <= max_len);
  LWIP_UNUSED_ARG(max_len);

  ret = dhcp6_option_short(options_out_len, options, DHCP6_OPTION_ORO);
  ret = dhcp6_option_short(ret, options, 2 * num_req_options);
  for (i = 0; i < num_req_options; i++) {
    ret = dhcp6_option_short(ret, options, req_options[i]);
  }
  return ret;
}

/* All options are added, shrink the pbuf to the required size */
static void
dhcp6_msg_finalize(u16_t options_out_len, struct pbuf *p_out)
{
  /* shrink the pbuf to the actual content length */
  pbuf_realloc(p_out, (u16_t)(sizeof(struct dhcp6_msg) + options_out_len));
}

static err_t
dhcp6_parse_client_id_opt(struct netif *netif, struct pbuf *p_msg_in, u8_t *client_id)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  u16_t op_start = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_CLI_ID); 
  u16_t op_len   = dhcp6_get_option_length(dhcp6, DHCP6_OPTION_IDX_CLI_ID);

  LWIP_UNUSED_ARG(dhcp6);

  if(DHCPV6_CLIENT_ID_LEN == op_len)
  {
      u16_t copied = pbuf_copy_partial(p_msg_in, (void*)client_id, NETIF_MAX_HWADDR_LEN, op_start+4);
      if (copied != NETIF_MAX_HWADDR_LEN) 
      {
        /* pbuf length mismatch */
        return ERR_BUF;
      }
  }
  else
  {
    return ERR_VAL;
  }
  return ERR_OK;  
}

static err_t
dhcp6_parse_server_id_opt(struct netif *netif, struct pbuf *p_msg_in, u8_t **server_id, u16_t *serverLen)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  u8_t *pServerID = NULL;
  u16_t op_start  = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_SERVER_ID); 
  u16_t op_len    = dhcp6_get_option_length(dhcp6, DHCP6_OPTION_IDX_SERVER_ID);

  LWIP_UNUSED_ARG(dhcp6);

  if(op_len >= DHCPV6_SERVER_ID_LEN_MAX)
  {
    return ERR_BUF;
  }
  pServerID = (u8_t*)malloc(op_len);
  if(NULL == pServerID)
  {
    return ERR_BUF;
  }
  u16_t copied = pbuf_copy_partial(p_msg_in, (void*)pServerID, op_len, op_start);
  if (copied != op_len) {
    free(pServerID);
    return ERR_VAL;
  }

  *serverLen = op_len;
  *server_id = pServerID;
  return ERR_OK;  
}

static err_t
dhcp6_parse_ia_na_opt(struct netif *netif, struct pbuf *p_msg_in, 
                      u32_t *IA_ID, u32_t *T1, u32_t *T2, u32_t *prefer, u32_t *valid, ip6_addr_t *net_addr)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  u16_t copied   = 0;
  u16_t op_start = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_IA_NA); 
  u16_t op_len   = dhcp6_get_option_length(dhcp6, DHCP6_OPTION_IDX_IA_NA);

  LWIP_UNUSED_ARG(dhcp6);

  if(DHCPV6_IANA_WITH_ADDR_LEN == op_len || DHCPV6_IANA_WITH_ADDR_STATUS_LEN == op_len){
    copied = pbuf_copy_partial(p_msg_in, (void*)IA_ID, DHCPV6_IA_ID_LEN, op_start);
    if (copied != DHCPV6_IA_ID_LEN) {
    return ERR_VAL;
    }
    op_start += DHCPV6_IA_ID_LEN;
    copied = pbuf_copy_partial(p_msg_in, (void*)T1, DHCPV6_T1_LEN, op_start);
    if (copied != DHCPV6_T1_LEN) {
    return ERR_VAL;
    }
    op_start += DHCPV6_T1_LEN;
    copied = pbuf_copy_partial(p_msg_in, (void*)T2, DHCPV6_T2_LEN, op_start);
    if (copied != DHCPV6_T2_LEN) {
    return ERR_VAL;
    }
    op_start += DHCPV6_T2_LEN;
    op_start += DHCPV6_OPT_HEAD_LEN;
    copied = pbuf_copy_partial(p_msg_in, net_addr->addr, sizeof(struct ip6_addr_packed), op_start);
    if (copied != sizeof(struct ip6_addr_packed)) {
      return ERR_VAL;
    }
    op_start += sizeof(struct ip6_addr_packed);
    copied = pbuf_copy_partial(p_msg_in, (void*)prefer, DHCPV6_PERFER_TIMER_LEN, op_start);
    if (copied != DHCPV6_PERFER_TIMER_LEN) {
    return ERR_VAL;
    }
    op_start += DHCPV6_PERFER_TIMER_LEN;
    copied = pbuf_copy_partial(p_msg_in, (void*)valid, DHCPV6_VALID_TIMER_LEN, op_start);
    if (copied != DHCPV6_VALID_TIMER_LEN) {
    return ERR_VAL;
    }
    *IA_ID  = lwip_ntohl(*IA_ID);
    *T1     = lwip_ntohl(*T1);
    *T2     = lwip_ntohl(*T2);
    *prefer = lwip_ntohl(*prefer);
    *valid  = lwip_ntohl(*valid);
  }
  else{
    return ERR_VAL;
  }
  return ERR_OK;
}

#if 0 // current DHCPServer do not give Status Option.
static err_t
dhcp6_parse_status_opt(struct netif *netif, struct pbuf *p_msg_in, u16_t *err)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  u16_t op_start = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_STATUS); 

  LWIP_UNUSED_ARG(dhcp6);

  u16_t copied = pbuf_copy_partial(p_msg_in, (void*)err, DHCPV6_STATUS_CODE_LEN, op_start);
  if (copied != DHCPV6_STATUS_CODE_LEN) 
  {
    /* pbuf length mismatch */
    return ERR_BUF;
  }
  *err = lwip_ntohs(*err); 
  
  return ERR_OK;  
}
#endif

#if LWIP_DHCP6_PROVIDE_DNS_SERVERS
static void
dhcp6_save_dns_opt(struct netif *netif, struct pbuf *p_msg_in)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  ip_addr_t dns_addr;
  ip6_addr_t *dns_addr6;
  u16_t op_start = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER);
  u16_t op_len = dhcp6_get_option_length(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER);
  u16_t idx;
  u8_t n;

  LWIP_UNUSED_ARG(dhcp6);

  memset(&dns_addr, 0, sizeof(dns_addr));
  dns_addr6 = ip_2_ip6(&dns_addr);
  for (n = LWIP_DHCP6_DNS_SERVERS, idx = op_start; (idx < op_start + op_len) && (n - LWIP_DHCP6_DNS_SERVERS < LWIP_DHCP6_PROVIDE_DNS_SERVERS);
        n++, idx += sizeof(struct ip6_addr_packed)) {
    u16_t copied = pbuf_copy_partial(p_msg_in, dns_addr6, sizeof(struct ip6_addr_packed), idx);
    if (copied != sizeof(struct ip6_addr_packed)) {
      /* pbuf length mismatch */
      return;
    }

    IP_SET_TYPE(&dns_addr, IPADDR_TYPE_V6);
    ip6_addr_assign_zone(dns_addr6, IP6_UNKNOWN, netif);
    
    /* @todo: do we need a different offset than DHCP(v4)? */
    dns_setserver(n, &dns_addr);
  }
}
#endif

#if LWIP_DHCP6_GET_NTP_SRV
static void
dhcp6_save_ntp_opt(struct netif *netif, struct pbuf *p_msg_in)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  ip_addr_t ntp_server_addrs[LWIP_DHCP6_MAX_NTP_SERVERS];
  u16_t op_start = dhcp6_get_option_start(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER);
  u16_t op_len = dhcp6_get_option_length(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER);
  u16_t idx;
  u8_t n;

  LWIP_UNUSED_ARG(dhcp6);

  for (n = 0, idx = op_start; (idx < op_start + op_len) && (n < LWIP_DHCP6_MAX_NTP_SERVERS);
        n++, idx += sizeof(struct ip6_addr_packed)) {
    u16_t copied;
    ip6_addr_t *ntp_addr6 = ip_2_ip6(&ntp_server_addrs[n]);
    ip_addr_set_zero_ip6(&ntp_server_addrs[n]);
    copied = pbuf_copy_partial(p_msg_in, ntp_addr6, sizeof(struct ip6_addr_packed), idx);
    if (copied != sizeof(struct ip6_addr_packed)) {
      /* pbuf length mismatch */
      return;
    }
    IP_SET_TYPE(&ntp_server_addrs[n], IPADDR_TYPE_V6);
    ip6_addr_assign_zone(ntp_addr6, IP6_UNKNOWN, netif);
  }
  dhcp6_set_ntp_servers(n, ntp_server_addrs);
}
#endif

#if LWIP_IPV6_DHCP6_STATELESS
static void
dhcp6_information_request(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t requested_options[] = {DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  u16_t msecs;
  struct pbuf *p_out;
  u16_t options_out_len;
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_information_request()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_INFOREQUEST, 4 + sizeof(requested_options), &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_information_request: making request\n"));

    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
      LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_REQUESTING_CONFIG, msg_out,
      DHCP6_INFOREQUEST, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_information_request: INFOREQUESTING -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_information_request: could not allocate DHCP6 request\n"));
  }
  dhcp6_set_state(dhcp6, DHCP6_STATE_REQUESTING_CONFIG, "dhcp6_information_request");
  if (dhcp6->tries < 255) {
    dhcp6->tries++;
  }
  msecs = (u16_t)((dhcp6->tries < 6 ? 1 << dhcp6->tries : 60) * 1000);
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_information_request(): set request timeout %"U16_F" msecs\n", msecs));
}

#if 0
static err_t
dhcp6_request_config(struct netif *netif, struct dhcp6 *dhcp6)
{
  /* stateless mode enabled and no request running? */
  if (dhcp6->state == DHCP6_STATE_STATELESS_IDLE) {
    /* send Information-request and wait for answer; setup receive timeout */
    dhcp6_information_request(netif, dhcp6);
  }

  return ERR_OK;
}

static void
dhcp6_abort_config_request(struct dhcp6 *dhcp6)
{
  if (dhcp6->state == DHCP6_STATE_REQUESTING_CONFIG) {
    /* abort running request */
    dhcp6_set_state(dhcp6, DHCP6_STATE_STATELESS_IDLE, "dhcp6_abort_config_request");
  }
}
#endif

/* Handle a REPLY to INFOREQUEST
 * This parses DNS and NTP server addresses from the reply.
 */
static void
dhcp6_stateless_handle_config_reply(struct netif *netif, struct pbuf *p_msg_in)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);

  LWIP_UNUSED_ARG(dhcp6);
  LWIP_UNUSED_ARG(p_msg_in);

#if LWIP_DHCP6_PROVIDE_DNS_SERVERS
  if (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER)) {
    dhcp6_save_dns_opt(netif, p_msg_in);
  }
  /* @ todo: parse and set Domain Search List */
#endif /* LWIP_DHCP6_PROVIDE_DNS_SERVERS */

#if LWIP_DHCP6_GET_NTP_SRV
  if (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER)) {
    dhcp6_save_ntp_opt(netif, p_msg_in);
  }
#endif /* LWIP_DHCP6_GET_NTP_SRV */
}
#endif /* LWIP_IPV6_DHCP6_STATELESS */

/** This function is called from nd6 module when an RA messsage is received
 * It triggers DHCPv6 requests (if enabled).
 */
static u8_t g_mflag = 0;
static u8_t g_oflag = 0;

void
dhcp6_nd6_ra_trigger(struct netif *netif, u8_t managed_addr_config, u8_t other_config)
{
  LWIP_ASSERT("netif != NULL", netif != NULL);
  
  	/*M 	O		type
	  0		0		slaac
	  0		1		stateless
	  1		x		stateful*/

  if (managed_addr_config == g_mflag 
  	&& other_config == g_oflag)
  	return;
  
  g_mflag = managed_addr_config;
  g_oflag = other_config;
  
  if (managed_addr_config)
  {
	dhcp6_enable_stateful(netif);
  }
  else if (other_config)
  {
	dhcp6_enable_stateless(netif);
  }
  else
  {
  	dhcp6_release_and_stop(netif);
  }

#if 0
  struct dhcp6 *dhcp6;
  dhcp6 = netif_dhcp6_data(netif);

  LWIP_UNUSED_ARG(managed_addr_config);
  LWIP_UNUSED_ARG(other_config);
  LWIP_UNUSED_ARG(dhcp6);

#if LWIP_IPV6_DHCP6_STATELESS
  if (dhcp6 != NULL) {
    if (dhcp6_stateless_enabled(dhcp6)) {
      if (other_config) {
        dhcp6_request_config(netif, dhcp6);
      } else {
        dhcp6_abort_config_request(dhcp6);
      }
    }
  }
#endif /* LWIP_IPV6_DHCP6_STATELESS */
#endif
}

#if LWIP_IPV6_DHCP6_STATEFUL
static err_t
dhcp6_stateful_handle_advertise(struct netif *netif, struct pbuf *p_msg_in)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);

  LWIP_UNUSED_ARG(dhcp6);
  LWIP_UNUSED_ARG(p_msg_in);

#if 0 // current DHCPServer do not give Status Option.
  //DHCP6_OPTION_STATUS_CODE exist and parsed successfully, status!=OK then release and restart.
  if(dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_STATUS))
  {
    u16_t status = ERR_OK;
    if( (ERR_OK == dhcp6_parse_status_opt(netif, p_msg_in, &status))
      &&(ERR_OK != status))
    {
      dhcp6_release_and_stop(netif);
      dhcp6_enable_stateful(netif);
      return ERR_VAL;
    }
  }
#endif

  if((dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_SERVER_ID))  
    && (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_IA_NA))
    && (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_CLI_ID)))
  {
    // msg's CLI_ID == staMAC
    u8_t  hwaddr[NETIF_MAX_HWADDR_LEN];
    if( (ERR_OK == dhcp6_parse_client_id_opt(netif, p_msg_in, hwaddr))
      &&(0 != memcmp(hwaddr, netif->hwaddr, NETIF_MAX_HWADDR_LEN)))
    {
      return ERR_VAL;
    }
	LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 adv mac match:%x,%x,%x,%x,%x,%x\n", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]));
    
    // get serverID 
    u8_t *pServerID = NULL;
    u16_t serverLen = 0;
    if (ERR_OK != dhcp6_parse_server_id_opt(netif, p_msg_in, &pServerID, &serverLen)) {
      return ERR_VAL;
    }

    // (0< T1 < T2 < pref < valid)
    u32_t IA_ID, T1, T2, prefer, valid;
    ip6_addr_t net_addr;
    if(ERR_OK != dhcp6_parse_ia_na_opt(netif, p_msg_in, &IA_ID, &T1, &T2, &prefer, &valid, &net_addr))
    {
      if(NULL != pServerID)
        free(pServerID);
      return ERR_VAL;
    }
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 adv IAID:%d, T1:%d, T2:%d, Prefer:%d, valid:%d\r\n", IA_ID, T1, T2, prefer, valid));
    if((IA_ID==1) && (T1<T2) && (T2<prefer) && (prefer<=valid))
    {
      u8_t idx;
      u8_t empty = 0xff;
      for(idx=0; idx<LWIP_IPV6_NUM_ADDRESSES; idx++)
      {
        if((netif->ip6_addr_state[idx] == IP6_ADDR_INVALID)
            && (empty == 0xff))
        {
          empty = idx;
        }
        if(ip6_addr_cmp_zoneless(ip_2_ip6(&netif->ip6_addr[empty]), &net_addr))
        {
          break;
        }
      }
      if((idx<LWIP_IPV6_NUM_ADDRESSES) || (empty==0xff))
      {
        LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 adv empty:%x, duplicateIP:%x,%x,%x,%x\r\n", empty, net_addr.addr[0], net_addr.addr[1], net_addr.addr[2], net_addr.addr[3]));
        if(NULL != pServerID)
          free(pServerID);
        return ERR_BUF;
      }
      //T1 T2 change to DHCP6 client timer tick
      dhcp6->T1_time  = T1*1000/DHCP6_TIMER_MSECS;
      dhcp6->T2_time  = T2*1000/DHCP6_TIMER_MSECS;
      dhcp6->addr_idx = empty;
      netif->ip6_addr_pref_life[empty]  = prefer;
      netif->ip6_addr_valid_life[empty] = valid;
      netif->ip6_dhcp_serverid[empty]   = pServerID;
      netif->ip6_dhcp_serverlen[empty]  = serverLen;
      ip6_addr_set(ip_2_ip6(&netif->ip6_addr[empty]), &net_addr);
      ip6_addr_assign_zone(ip_2_ip6(&netif->ip6_addr[empty]), IP6_UNICAST, netif);
      netif_ip6_addr_set_state(netif, empty, IP6_ADDR_TENTATIVE);
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 adv [%d]IP:%x,%x,%x,%x\r\n", empty, net_addr.addr[0], net_addr.addr[1], net_addr.addr[2], net_addr.addr[3]));
      return ERR_OK;
    }
    else
    {
      if(NULL != pServerID)
        free(pServerID);
      return ERR_VAL;
    }
  }
  else
  {
    return ERR_VAL;
  }
  return ERR_OK;
}

static err_t
dhcp6_stateful_handle_reply(struct netif *netif, struct pbuf *p_msg_in)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);

  LWIP_UNUSED_ARG(dhcp6);
  LWIP_UNUSED_ARG(p_msg_in);

#if 0 // current DHCPServer do not give Status Option.
  //DHCP6_OPTION_STATUS_CODE exist and parsed successfully, status!=OK then release and restart.
  if(dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_STATUS))
  {
    u16_t status = ERR_OK;
    if( (ERR_OK == dhcp6_parse_status_opt(netif, p_msg_in, &status))
      &&(ERR_OK != status))
    {
      dhcp6_release_and_stop(netif);
      dhcp6_enable_stateful(netif);
      return ERR_VAL;
    }
  }
#endif

  if((dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_SERVER_ID))  
  && (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_IA_NA))
  && (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_CLI_ID)))
  {
    // msg's CLI_ID == staMAC
    u8_t  hwaddr[NETIF_MAX_HWADDR_LEN];
    if( (ERR_OK == dhcp6_parse_client_id_opt(netif, p_msg_in, hwaddr))
      &&(0 != memcmp(hwaddr, netif->hwaddr, NETIF_MAX_HWADDR_LEN)))
    {
      return ERR_VAL;
    }
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply mac match:%x,%x,%x,%x,%x,%x\r\n", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]));

    // get serverID 
    u8_t *pServerID = NULL;
    u16_t serverLen = 0;
    if (ERR_OK != dhcp6_parse_server_id_opt(netif, p_msg_in, &pServerID, &serverLen)) {
      return ERR_VAL;
    }

    // (0< T1 < T2 < pref < valid) && (IAAddr is Golbal)
    u32_t      IA_ID, T1, T2, prefer, valid;
    ip6_addr_t net_addr;
    if(ERR_OK != dhcp6_parse_ia_na_opt(netif, p_msg_in, &IA_ID, &T1, &T2, &prefer, &valid, &net_addr))
    {
      if(NULL != pServerID)
        free(pServerID);
      return ERR_VAL;
    }
    if((IA_ID==1) && (T1<T2) && (T2<prefer) && (prefer<=valid))
    {
      u8_t idx;

      for(idx=0; idx<LWIP_IPV6_NUM_ADDRESSES; idx++)
      {
        if(ip6_addr_cmp_zoneless(ip_2_ip6(&netif->ip6_addr[idx]), &net_addr))
        {
          break;
        }
      }
      if(idx >= LWIP_IPV6_NUM_ADDRESSES)
      {
        LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply IP not exist:%x,%x,%x,%x\r\n", net_addr.addr[0], net_addr.addr[1], net_addr.addr[2], net_addr.addr[3]));
        if(NULL != pServerID)
          free(pServerID);
        return ERR_BUF;
      }
      if(dhcp6->state == DHCP6_STATE_STATEFUL_REBIND)
      {
        if(NULL != netif->ip6_dhcp_serverid[idx])
        {
          free(netif->ip6_dhcp_serverid[idx]);
          netif->ip6_dhcp_serverid[idx] = pServerID;
        }
        else
        {
          free(pServerID);
          return ERR_BUF;
        }
      }
      else if(0 != memcmp(pServerID, netif->ip6_dhcp_serverid[idx], netif->ip6_dhcp_serverlen[idx]))
      {
        LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply Server not exist:%x,%x,%x,%x\r\n", net_addr.addr[0], net_addr.addr[1], net_addr.addr[2], net_addr.addr[3]));
        if(NULL != pServerID)
          free(pServerID);
        return ERR_BUF;
      }
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply IP:%x,%x,%x,%x\r\n", net_addr.addr[0], net_addr.addr[1], net_addr.addr[2], net_addr.addr[3]));
      //T1 T2 change to DHCP6 client timer tick
      dhcp6->T1_time  = T1*1000/DHCP6_TIMER_MSECS;
      dhcp6->T2_time  = T2*1000/DHCP6_TIMER_MSECS;
      dhcp6->addr_idx = idx;
      netif->ip6_addr_pref_life[idx]  = prefer;
      netif->ip6_addr_valid_life[idx] = valid;
      if(dhcp6->state != DHCP6_STATE_STATEFUL_REBIND)
      {
          if(NULL != pServerID)
          free(pServerID);
      }
    }
    else
    {
      if(NULL != pServerID)
        free(pServerID);
      return ERR_VAL; 
    }
#if LWIP_DHCP6_PROVIDE_DNS_SERVERS
    //save DNS
    if (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER)) {
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply DNS DNS\r\n"));
      dhcp6_save_dns_opt(netif, p_msg_in);
    }
#endif /* LWIP_DHCP6_PROVIDE_DNS_SERVERS */

#if LWIP_DHCP6_GET_NTP_SRV
    //save NTP
    if (dhcp6_option_given(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER)) {
      LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 reply NTP NTP\r\n"));
      dhcp6_save_ntp_opt(netif, p_msg_in);
    }
#endif /* LWIP_DHCP6_GET_NTP_SRV */
  }
  else
  {
    return ERR_VAL;
  }
  return ERR_OK;
}

static void
dhcp6_stateful_solicit(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_SOL_MAX_RT};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  
  
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_solicit()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_SOLICIT, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_solicit: making solicit\n"));

    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_NO_ADDR_LEN, 0xFF, 0, 0, 0, 0, p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
      LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_SOLICIT, msg_out,
      DHCP6_SOLICIT, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 solicit ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_solicit: solicit -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_solicit: could not allocate DHCP6 request\n"));
  }
  msecs = ( (DHCPV6_TIMER_SOL_TIMEOUT<<dhcp6->tries)<DHCPV6_TIMER_SOL_MAX_RT ? (DHCPV6_TIMER_SOL_TIMEOUT<<dhcp6->tries) : DHCPV6_TIMER_SOL_MAX_RT) * 1000;
  dhcp6->request_timeout = (u16_t)( (msecs+DHCP6_TIMER_MSECS-1)/DHCP6_TIMER_MSECS );
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 solicit retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_solicit(): set solicit timeout %"U16_F" msecs\n", msecs));
}

static void
dhcp6_stateful_request(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_SOL_MAX_RT, DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Server ID 
  opt_len += netif->ip6_dhcp_serverlen[dhcp6->addr_idx]+DHCPV6_OPT_HEAD_LEN; 
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  
  
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_request()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_REQUEST, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_request: making request\n"));

    options_out_len = dhcp6_option_server_id(options_out_len, options, netif->ip6_dhcp_serverid[dhcp6->addr_idx], 
                                             netif->ip6_dhcp_serverlen[dhcp6->addr_idx], p_out->len);
    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_WITH_ADDR_LEN, dhcp6->addr_idx, dhcp6->T1_time, dhcp6->T2_time, 
                                         netif->ip6_addr_pref_life[dhcp6->addr_idx], netif->ip6_addr_valid_life[dhcp6->addr_idx], p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
                                                 LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_REQUEST, msg_out,
      DHCP6_REQUEST, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 request ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_request: request -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_request: could not allocate DHCP6 request\n"));
  }

  if(dhcp6->tries >= DHCPV6_TIMER_REQ_MAX_RC)
  {
    dhcp6_release_and_stop(netif);
    dhcp6_enable_stateful(netif);
  }
  msecs = ( (DHCPV6_TIMER_REQ_TIMEOUT<<dhcp6->tries)<DHCPV6_TIMER_REQ_MAX_RT ? (DHCPV6_TIMER_REQ_TIMEOUT<<dhcp6->tries) : DHCPV6_TIMER_REQ_MAX_RT) * 1000;
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 request retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_request(): set request timeout %"U16_F" msecs\n", msecs));
}

static void
dhcp6_stateful_renew(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Server ID 
  opt_len += netif->ip6_dhcp_serverlen[dhcp6->addr_idx]+DHCPV6_OPT_HEAD_LEN; 
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  
  
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_renew()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_RENEW, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_renew: making request\n"));

    options_out_len = dhcp6_option_server_id(options_out_len, options, netif->ip6_dhcp_serverid[dhcp6->addr_idx], 
                                             netif->ip6_dhcp_serverlen[dhcp6->addr_idx], p_out->len);
    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_WITH_ADDR_LEN, dhcp6->addr_idx, 0, 0, 0, 0, p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
                                                 LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_RENEW, msg_out,
      DHCP6_RENEW, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 renew ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_renew: request -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_renew: could not allocate DHCP6 request\n"));
  }

  msecs = ( (DHCPV6_TIMER_REN_TIMEOUT<<dhcp6->tries)<DHCPV6_TIMER_REN_MAX_RT ? (DHCPV6_TIMER_REN_TIMEOUT<<dhcp6->tries) : DHCPV6_TIMER_REN_MAX_RT) * 1000;
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 renew retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_renew(): set request timeout %"U16_F" msecs\n", msecs));
}

static void
dhcp6_stateful_rebind(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  
  
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_rebind()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_REBIND, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_rebind: making request\n"));

    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_WITH_ADDR_LEN, dhcp6->addr_idx, 0, 0, 0, 0, p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
                                                 LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_REBIND, msg_out,
      DHCP6_REBIND, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 rebind ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_rebind: request -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_rebind: could not allocate DHCP6 request\n"));
  }

  msecs = ( (DHCPV6_TIMER_REB_TIMEOUT<<dhcp6->tries)<DHCPV6_TIMER_REB_MAX_RT ? (DHCPV6_TIMER_REB_TIMEOUT<<dhcp6->tries) : DHCPV6_TIMER_REB_MAX_RT) * 1000;
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 rebind retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_renew(): set request timeout %"U16_F" msecs\n", msecs));
}

static void
dhcp6_stateful_release(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Server ID 
  opt_len += netif->ip6_dhcp_serverlen[dhcp6->addr_idx]+DHCPV6_OPT_HEAD_LEN; 
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  

  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_release()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_RELEASE, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_release: making request\n"));

    options_out_len = dhcp6_option_server_id(options_out_len, options, netif->ip6_dhcp_serverid[dhcp6->addr_idx], 
                                             netif->ip6_dhcp_serverlen[dhcp6->addr_idx], p_out->len);
    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_WITH_ADDR_LEN, dhcp6->addr_idx, 0, 0, 0, 0, p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
                                                 LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_RELEASE, msg_out,
      DHCP6_RELEASE, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 release ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_release: request -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_release: could not allocate DHCP6 request\n"));
  }

  msecs = (DHCPV6_TIMER_REL_TIMEOUT<<dhcp6->tries) * 1000;
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 release retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_release(): set request timeout %"U16_F" msecs\n", msecs));
}

static void
dhcp6_stateful_decline(struct netif *netif, struct dhcp6 *dhcp6)
{
  const u16_t  requested_options[] = {DHCP6_OPTION_DNS_SERVERS, DHCP6_OPTION_DOMAIN_LIST, DHCP6_OPTION_SNTP_SERVERS};
  struct pbuf *p_out               = NULL;
  u32_t        msecs               = 0;
  u16_t        options_out_len     = 0;
  u16_t        opt_len             = 0;
  
  // Server ID 
  opt_len += netif->ip6_dhcp_serverlen[dhcp6->addr_idx]+DHCPV6_OPT_HEAD_LEN; 
  // Client ID
  opt_len += DHCPV6_CLIENT_ID_LEN+DHCPV6_OPT_HEAD_LEN;                       
  // ELAPSED_TIME
  opt_len += DHCPV6_ELAPSED_TIME_LEN+DHCPV6_OPT_HEAD_LEN;                    
  // IANA
  opt_len += DHCPV6_IANA_WITH_ADDR_LEN+DHCPV6_OPT_HEAD_LEN;                  
  // ORO
  opt_len += sizeof(requested_options)+DHCPV6_OPT_HEAD_LEN;                  
  
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_decline()\n"));
  /* create and initialize the DHCP message header */
  p_out = dhcp6_create_msg(netif, dhcp6, DHCP6_DECLINE, opt_len, &options_out_len);
  if (p_out != NULL) {
    err_t err;
    struct dhcp6_msg *msg_out = (struct dhcp6_msg *)p_out->payload;
    u8_t *options = (u8_t *)(msg_out + 1);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_stateful_decline: making request\n"));

    options_out_len = dhcp6_option_server_id(options_out_len, options, netif->ip6_dhcp_serverid[dhcp6->addr_idx], 
                                             netif->ip6_dhcp_serverlen[dhcp6->addr_idx], p_out->len);
    options_out_len = dhcp6_option_client_id(options_out_len, options, (const u8_t*)netif->hwaddr, p_out->len);
    options_out_len = dhcp6_option_elapsed_time(options_out_len, options, dhcp6->elapsed_time, p_out->len);
    options_out_len = dhcp6_option_ia_na(options_out_len, options, netif, DHCPV6_IANA_WITH_ADDR_LEN, dhcp6->addr_idx, 0, 0, 0, 0, p_out->len);
    options_out_len = dhcp6_option_optionrequest(options_out_len, options, requested_options,
                                                 LWIP_ARRAYSIZE(requested_options), p_out->len);
    LWIP_HOOK_DHCP6_APPEND_OPTIONS(netif, dhcp6, DHCP6_STATE_STATEFUL_DECLINE, msg_out,
      DHCP6_DECLINE, options_out_len, p_out->len);
    dhcp6_msg_finalize(options_out_len, p_out);

    err = udp_sendto_if(dhcp6_pcb, p_out, &dhcp6_All_DHCP6_Relay_Agents_and_Servers, DHCP6_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 decline ret:%d\r\n", err));
    pbuf_free(p_out);
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_decline: request -> %d\n", (int)err));
    LWIP_UNUSED_ARG(err);
  } else {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp6_stateful_decline: could not allocate DHCP6 request\n"));
  }

  msecs = (DHCPV6_TIMER_DEC_TIMEOUT<<dhcp6->tries) * 1000;
  dhcp6->request_timeout = (u16_t)((msecs + DHCP6_TIMER_MSECS - 1) / DHCP6_TIMER_MSECS);
  if (dhcp6->tries < DHCPV6_RETRY_MAX) {
    dhcp6->tries++;
  }
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 decline retry:%d, time:%d\r\n", dhcp6->tries, dhcp6->request_timeout));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_stateful_decline(): set request timeout %"U16_F" msecs\n", msecs));
}
#endif

void
dhcp6_release_and_stop(struct netif *netif)
{
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);

  if((NULL == dhcp6) || (dhcp6->state == DHCP6_STATE_OFF))
  {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("release dhcp6:%x.\r\n", dhcp6));
    return;
  }

  if (dhcp6_stateful_enabled(dhcp6))
  {
	  dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_RELEASE, "release_and_stop");
	  dhcp6_stateful_release(netif, dhcp6);
	  
	  for(u8_t idx=0; idx<LWIP_IPV6_NUM_ADDRESSES; idx++)
	  {
	      if(!ip6_addr_islinklocal(ip_2_ip6(&(netif->ip6_addr[idx]))))
	      {
	          netif_ip6_addr_set_valid_life(netif, idx, 0);
	          netif_ip6_addr_set_pref_life(netif, idx, 0);
	          netif_ip6_addr_set_state(netif, idx, IP6_ADDR_INVALID);
	          IP_ADDR6(&(netif->ip6_addr[idx]), 0, 0, 0, 0);
	          netif->ip6_dhcp_serverlen[idx]  = 0;
	          if(NULL != netif->ip6_dhcp_serverid[idx])
	          {
	            free(netif->ip6_dhcp_serverid[idx]);
	            netif->ip6_dhcp_serverid[idx] = NULL;
	          }
	      }
	  }

	  dhcp6->addr_idx        = 0xFF;
	  dhcp6->tries           = 0;
	  dhcp6->xid             = 0;
	  dhcp6->request_timeout = 0;
	  dhcp6->elapsed_time    = 0;
	  dhcp6->T1_time         = 0;
	  dhcp6->T2_time         = 0;
  }

  dhcp6_disable(netif);
}

/**
 * Parse the DHCPv6 message and extract the DHCPv6 options.
 *
 * Extract the DHCPv6 options (offset + length) so that we can later easily
 * check for them or extract the contents.
 */
static err_t
dhcp6_parse_reply(struct pbuf *p, struct dhcp6 *dhcp6)
{
  u16_t offset;
  u16_t offset_max;
  u16_t options_idx;
  struct dhcp6_msg *msg_in;

  LWIP_UNUSED_ARG(dhcp6);

  /* clear received options */
  dhcp6_clear_all_options(dhcp6);
  msg_in = (struct dhcp6_msg *)p->payload;

  /* parse options */

  options_idx = sizeof(struct dhcp6_msg);
  /* parse options to the end of the received packet */
  offset_max = p->tot_len;

  offset = options_idx;
  /* at least 4 byte to read? */
  while ((offset + 4 <= offset_max)) {
    u8_t op_len_buf[4];
    u8_t *op_len;
    u16_t op;
    u16_t len;
    u16_t val_offset = (u16_t)(offset + 4);
    if (val_offset < offset) {
      /* overflow */
      return ERR_BUF;
    }
    /* copy option + length, might be split accross pbufs */
    op_len = (u8_t *)pbuf_get_contiguous(p, op_len_buf, 4, 4, offset);
    if (op_len == NULL) {
      /* failed to get option and length */
      return ERR_VAL;
    }
    op = (op_len[0] << 8) | op_len[1];
    len = (op_len[2] << 8) | op_len[3];
    offset = val_offset + len;
    if (offset < val_offset) {
      /* overflow */
      return ERR_BUF;
    }

    switch (op) {
      case (DHCP6_OPTION_CLIENTID):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_CLI_ID);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_CLI_ID, val_offset, len);
        break;
      case (DHCP6_OPTION_SERVERID):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_SERVER_ID);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_SERVER_ID, val_offset, len);
        break;
		case (DHCP6_OPTION_STATUS_CODE):
		  dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_STATUS);
		  dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_STATUS, val_offset, len);
		  break;
      case (DHCP6_OPTION_IA_NA):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_IA_NA);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_IA_NA, val_offset, len);
        break;
#if LWIP_DHCP6_PROVIDE_DNS_SERVERS
      case (DHCP6_OPTION_DNS_SERVERS):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_DNS_SERVER, val_offset, len);
        break;
      case (DHCP6_OPTION_DOMAIN_LIST):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_DOMAIN_LIST);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_DOMAIN_LIST, val_offset, len);
        break;
#endif /* LWIP_DHCP6_PROVIDE_DNS_SERVERS */
#if LWIP_DHCP6_GET_NTP_SRV
      case (DHCP6_OPTION_SNTP_SERVERS):
        dhcp6_got_option(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER);
        dhcp6_set_option(dhcp6, DHCP6_OPTION_IDX_NTP_SERVER, val_offset, len);
        break;
#endif /* LWIP_DHCP6_GET_NTP_SRV*/
      default:
        LWIP_DEBUGF(DHCP6_DEBUG, ("skipping option %"U16_F" in options\n", op));
        LWIP_HOOK_DHCP6_PARSE_OPTION(ip_current_netif(), dhcp6, dhcp6->state, msg_in,
          msg_in->msgtype, op, len, q, val_offset);
        break;
    }
  }
  return ERR_OK;
}

static void
dhcp6_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct netif *netif = ip_current_input_netif();
  struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
  struct dhcp6_msg *reply_msg = (struct dhcp6_msg *)p->payload;
  u8_t msg_type;
  u32_t xid;

  LWIP_UNUSED_ARG(arg);

  /* Caught DHCPv6 message from netif that does not have DHCPv6 enabled? -> not interested */
  if ((dhcp6 == NULL) || (dhcp6->pcb_allocated == 0)) {
    goto free_pbuf_and_return;
  }

  LWIP_ERROR("invalid server address type", IP_IS_V6(addr), goto free_pbuf_and_return;);

  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_recv(pbuf = %p) from DHCPv6 server %s port %"U16_F"\n", (void *)p,
    ipaddr_ntoa(addr), port));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("pbuf->len = %"U16_F"\n", p->len));
  LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("pbuf->tot_len = %"U16_F"\n", p->tot_len));
  /* prevent warnings about unused arguments */
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  if (p->len < sizeof(struct dhcp6_msg)) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("DHCPv6 reply message or pbuf too short\n"));
    goto free_pbuf_and_return;
  }

  /* match transaction ID against what we expected */
  xid = reply_msg->transaction_id[0] << 16;
  xid |= reply_msg->transaction_id[1] << 8;
  xid |= reply_msg->transaction_id[2];
  if (xid != dhcp6->xid) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
                ("transaction id mismatch reply_msg->xid(%"X32_F")!= dhcp6->xid(%"X32_F")\n", xid, dhcp6->xid));
    goto free_pbuf_and_return;
  }
  /* option fields could be unfold? */
  if (dhcp6_parse_reply(p, dhcp6) != ERR_OK) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
                ("problem unfolding DHCPv6 message - too short on memory?\n"));
    goto free_pbuf_and_return;
  }

  /* read DHCP message type */
  msg_type = reply_msg->msgtype;
  /* message type is DHCP6 REPLY? */
  switch (msg_type)
  {
  case DHCP6_REPLY:
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("DHCP6_REPLY received\n"));
    /* in info-requesting state? */
    if (dhcp6->state == DHCP6_STATE_REQUESTING_CONFIG) {
#if LWIP_IPV6_DHCP6_STATELESS
      dhcp6_set_state(dhcp6, DHCP6_STATE_STATELESS_IDLE, "dhcp6_recv");
      dhcp6_stateless_handle_config_reply(netif, p);
#endif /* LWIP_IPV6_DHCP6_STATELESS */
    } else if( (dhcp6->state == DHCP6_STATE_STATEFUL_REQUEST)
            || (dhcp6->state == DHCP6_STATE_STATEFUL_RENEW)
            || (dhcp6->state == DHCP6_STATE_STATEFUL_REBIND))
    { 
#if LWIP_IPV6_DHCP6_STATEFUL
      if(ERR_OK == dhcp6_stateful_handle_reply(netif, p))
      {
        dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_BOND, "dhcp6_Reply_recv");
      }
#endif /* LWIP_IPV6_DHCP6_STATEFUL */
    }
    break;
  case DHCP6_ADVERTISE:
    if(dhcp6->state == DHCP6_STATE_STATEFUL_SOLICIT){
#if LWIP_IPV6_DHCP6_STATEFUL
      if(ERR_OK == dhcp6_stateful_handle_advertise(netif, p)){
        dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_REQUEST, "dhcp6_ADV_recv");
        dhcp6_stateful_request(netif, dhcp6);
      }
#endif /* LWIP_IPV6_DHCP6_STATEFUL */
    }
    break;  
  default:
    break;
  }

free_pbuf_and_return:
  pbuf_free(p);
}

/**
 * A DHCPv6 request has timed out.
 *
 * The timer that was started with the DHCPv6 request has
 * timed out, indicating no response was received in time.
 */
static void
dhcp6_timeout(struct netif *netif, struct dhcp6 *dhcp6)
{
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp6_timeout()\n"));

  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(dhcp6);

#if LWIP_IPV6_DHCP6_STATELESS
  /* back-off period has passed, or server selection timed out */
  if (dhcp6->state == DHCP6_STATE_REQUESTING_CONFIG) {
    LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE, ("dhcp6_timeout(): retrying information request\n"));
    dhcp6_information_request(netif, dhcp6);
  }
#endif /* LWIP_IPV6_DHCP6_STATELESS */
#if LWIP_IPV6_DHCP6_STATEFUL
  switch (dhcp6->state)
  {
  case DHCP6_STATE_STATEFUL_SOLICIT:
    dhcp6_stateful_solicit(netif, dhcp6);
    break;
  case DHCP6_STATE_STATEFUL_REQUEST:
    dhcp6_stateful_request(netif, dhcp6);
    break;
  case DHCP6_STATE_STATEFUL_RENEW:
    dhcp6_stateful_renew(netif, dhcp6);
    break;
  case DHCP6_STATE_STATEFUL_REBIND:
    dhcp6_stateful_rebind(netif, dhcp6);
    break;
  default:
    break;
  }
#endif
}

/**
 * DHCPv6 timeout handling (this function must be called every 500ms,
 * see @ref DHCP6_TIMER_MSECS).
 *
 * A DHCPv6 server is expected to respond within a short period of time.
 * This timer checks whether an outstanding DHCPv6 request is timed out.
 */
void
dhcp6_tmr(void)
{
  struct netif *netif;

  /* loop through netif's */
  NETIF_FOREACH(netif) {
    struct dhcp6 *dhcp6 = netif_dhcp6_data(netif);
    /* only act on DHCPv6 configured interfaces */
    if (dhcp6 != NULL) {
      /* elapased timer for dhcp6 diag*/
      dhcp6->elapsed_time++;

      /* re-try timer is active (non zero), and is about to trigger now */
      if (dhcp6->request_timeout > 1) {
        dhcp6->request_timeout--;
      } else if (dhcp6->request_timeout == 1) {
        dhcp6->request_timeout--;
        /* { dhcp6->request_timeout == 0 } */
        LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6_tmr(): request timeout\n"));
        /* this client's request timeout triggered */
        dhcp6_timeout(netif, dhcp6);
      }

      /* T1 timer is active (non zero), and is about to trigger now */
      if(dhcp6->T1_time > 1){
        dhcp6->T1_time--;
      }
      else if(dhcp6->T1_time == 1){
        dhcp6->T1_time--;
        dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_RENEW, "T1 timeout");
        dhcp6_stateful_renew(netif, dhcp6);
      }

      /* T2 timer is active (non zero), and is about to trigger now */
      if(dhcp6->T2_time > 1){
        dhcp6->T2_time--;
      }
      else if(dhcp6->T2_time == 1){
        dhcp6->T2_time--;
        dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_REBIND, "T2 timeout");
        dhcp6_stateful_rebind(netif, dhcp6);
      }

#if LWIP_IPV6_DHCP6_STATEFUL
      /* The address in used is INVALID(by ND timer) or DUPLICATE(by DAD) */
      if((dhcp6->addr_idx != 0xFF) 
         &&((IP6_ADDR_DUPLICATED==netif_ip6_addr_state(netif, dhcp6->addr_idx))
            ||(IP6_ADDR_INVALID==netif_ip6_addr_state(netif, dhcp6->addr_idx))))
      {
        LWIP_DEBUGF(DHCP6_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp6 timer restart :%d\r\n", netif_ip6_addr_state(netif, dhcp6->addr_idx)));
        if(IP6_ADDR_DUPLICATED==netif_ip6_addr_state(netif, dhcp6->addr_idx))
        {
          dhcp6_set_state(dhcp6, DHCP6_STATE_STATEFUL_DECLINE, "DAD Failed");
          dhcp6_stateful_decline(netif, dhcp6);
        }
        dhcp6_release_and_stop(netif);
        dhcp6_enable_stateful(netif);
      }
#endif
    }
  }
}

#endif /* LWIP_IPV6 && LWIP_IPV6_DHCP6 */
