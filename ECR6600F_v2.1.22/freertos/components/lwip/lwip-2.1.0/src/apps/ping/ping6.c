/**
 * @file
 * Ping sender module
 *
 */

/*
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
 */

/**
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */
//#include "system.h"

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "apps/ping/ping.h"
#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp6.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

/**
 * PING6_DEBUG: Enable debugging for PING.
 */
#ifndef PING6_DEBUG
#define PING6_DEBUG     LWIP_DBG_ON
#endif

/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif


#ifdef LWIP_IPV6
#define false 0
#define true  1

#define LWIP_PING_TASK_PRIORITY	( configMAX_PRIORITIES - 6 )
#define LWIP_PING_TASK_STACK_SIZE	( 512 )

#endif

static ping_parm_t* ping_all_connections;

/** Report a ping test result */
static void 
ping_statistics_report(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
    double delay __attribute__((unused)) = (ping_info->success == 0) ? 0 : ((double)ping_info->total_delay/(double)ping_info->success);
	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG, ("\n---------------------------- Ping6 Statistics --------------------------------\n"));
	LWIP_DEBUGF( PING6_DEBUG, (" Interface : wlan%d                   Target Address : ", ping_info->vif_id));
	ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
	LWIP_DEBUGF( PING6_DEBUG, ("\n"));
	LWIP_DEBUGF( PING6_DEBUG, (" %d packets transmitted, %d received, %d%% packet loss, %d.%d ms average delay",\
		 ping_info->count, ping_info->success, ((ping_info->count-ping_info->success)*100/ping_info->count), \
		(int)delay, (int)((delay - (int)delay) * 10)));
	LWIP_DEBUGF( PING6_DEBUG, ("\n-----------------------------------------------------------------------------\n"));
	ping_mutex_unlock();
}

/** Initialize ping test parameters */
static void
ping_parameters_init(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;

	ping_info->seq_num = 0;
	ping_info->time = 0;
	ping_info->count = 0;
	ping_info->success = 0;
	ping_info->total_delay = 0;
}

/** Prepare a echo ICMP request */
static void
ping_prepare_echo(void *arg, struct icmp6_echo_hdr *iecho, u16_t len)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	size_t i;
	size_t data_len = len - sizeof(struct icmp6_echo_hdr);

	iecho->type = ICMP6_TYPE_EREQ;
	iecho->code = 0;
	iecho->chksum = 0;
	iecho->id     = PING_ID;
	iecho->seqno  = htons(++ping_info->seq_num);

	/* fill the additional data buffer with some data */
	for(i = 0; i < data_len; i++) {
		((char*)iecho)[sizeof(struct icmp6_echo_hdr) + i] = (char)i;
	}
}

/* Ping using the socket ip */
static err_t
ping_send(void *arg, int s, ip_addr_t *addr)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	int err;
	struct icmp6_echo_hdr *iecho;
	struct sockaddr_in6 to;
	size_t ping_size = sizeof(struct icmp6_echo_hdr) + ping_info->packet_size;

	#if CONFIG_ECR6600
    iecho = (struct icmp6_echo_hdr*)os_malloc((mem_size_t)ping_size);
	#elif
    iecho = (struct icmp6_echo_hdr*)mem_malloc((mem_size_t)ping_size);
	#endif
	if (!iecho) {
		return ERR_MEM;
	}

	ping_prepare_echo(ping_info, iecho, (u16_t)ping_size);

	to.sin6_len = sizeof(to);
	to.sin6_family = AF_INET6;
	to.sin6_flowinfo = 0;
	to.sin6_scope_id = ip6_addr_zone(ip_2_ip6(&ping_info->addr));
	inet6_addr_from_ip6addr(&to.sin6_addr, &(ping_info->addr.u_addr.ip6));

	err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr*)&to, sizeof(to));

	#if CONFIG_ECR6600
    os_free(iecho);
	#elif
    mem_free(iecho);
	#endif

	return (err ? ERR_OK : ERR_VAL);
}

static void
ping_recv(void *arg, int s)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	char buf[64];
	int fromlen = sizeof(struct sockaddr_in6), len;
	struct sockaddr_in6 from;
	struct icmp6_echo_hdr *iecho;

	ping_info->time_delay=0;

	while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) {
		if (len >= (int)(sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr))) {
			iecho = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);
			if ( ( iecho->id == PING_ID ) && ( iecho->seqno == htons(ping_info-> seq_num ) ) ){
				ping_info->time_delay = (sys_now() -ping_info->time);
				ip_addr_t fromaddr;
				IP_SET_TYPE(&fromaddr, IPADDR_TYPE_V6);
				inet6_addr_to_ip6addr(&fromaddr.u_addr.ip6, &from.sin6_addr);
				ping_mutex_lock();
				LWIP_DEBUGF( PING6_DEBUG, ("ping6: recv "));
				ip_addr_debug_print(PING6_DEBUG, &fromaddr);
				LWIP_DEBUGF( PING6_DEBUG, (" [%d] ", ping_info->seq_num));
				LWIP_DEBUGF( PING6_DEBUG, (" %"U32_F" ms\n", ping_info->time_delay));
				ping_mutex_unlock();
				/* do some ping result processing */
				PING_RESULT((iecho->type == ICMP_ER));
				ping_info->total_delay += ping_info->time_delay;
				ping_info->success++;
				return;
			}
		}
	}

	ping_mutex_lock();
	LWIP_DEBUGF( PING6_DEBUG, ("ping6: recv "));
	ip6_addr_debug_print_val(PING6_DEBUG, (ping_info->addr.u_addr.ip6));
	LWIP_DEBUGF( PING6_DEBUG, (" timeout\n"));
	ping_mutex_unlock();

	/* do some ping result processing */
	PING_RESULT(0);
}


/** Add an ping session to the 'active' list */
static void
ping_list_add(ping_parm_t* item)
{
	ping_parm_t* temp;
	if (ping_all_connections == NULL) {
		ping_all_connections = item;
		item->next = NULL;
	} else {
		temp = ping_all_connections;
		ping_all_connections = item;
		item->next = temp;
	}
}

/** Remove an ping session from the 'active' list */
static void
ping_list_remove(ping_parm_t* item)
{
	ping_parm_t* prev = NULL;
	ping_parm_t* iter;
	for (iter = ping_all_connections; iter != NULL; prev = iter, iter = iter->next) {
		if (iter == item) {
			if (prev == NULL) {
				ping_all_connections = iter->next;
			} else {
				prev->next = iter->next;
			}
			break;
		}
	}
}

ping_parm_t *
ping6_get_session(ip6_addr_t* addr)
{
	ping_parm_t* iter;

	for (iter = ping_all_connections; iter != NULL;iter = iter->next) 
	{
		if(ip6_addr_cmp(&(iter->addr.u_addr.ip6),addr))
		{
			return iter;
		}
	}
	return NULL;
}

void ping6_list_display(void) {
	ping_parm_t *ptr = ping_all_connections;

	if(ptr == NULL){
		ping_mutex_lock();
		os_printf(LM_APP, LL_INFO, "No ping application is running\n");
		ping_mutex_unlock();
		return;
	}

	ping_mutex_lock();
	os_printf(LM_APP, LL_INFO, "\n-------------------------- Ping6 running Status ------------------------------\n");
	while(ptr != NULL) {
		os_printf(LM_APP, LL_INFO, "vif_id: %d\t ", ptr->vif_id);
		os_printf(LM_APP, LL_INFO, "%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F ":%" X16_F"\n", \
                      IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), \
                      IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), \
                      IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)), IP6_ADDR_BLOCK1(&(ptr->addr.u_addr.ip6)));

		ptr = ptr->next;
	}
	os_printf(LM_APP, LL_INFO, "-----------------------------------------------------------------------------\n");
	ping_mutex_unlock();
}

static void
ping_thread(void *arg)
{
	int s;
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	int timeout = ping_info->rec_timeout;

	if ((s = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6)) < 0) {
		ping6_deinit(ping_info);
		//if(!ping_info)
		//	vPortFree(ping_info);
		return;
	}
	struct ifreq iface;
	struct netif *netif = netif_get_by_index(ping_info->vif_id + 1);
	iface.ifr_name[0] = netif->name[0];
	iface.ifr_name[1] = netif->name[1];
	iface.ifr_name[2] = '0' + netif->num;
	
	lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	lwip_setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(iface));
	ping_parameters_init(ping_info);
	ping_list_add(ping_info);
	IP_SET_TYPE(&ping_info->addr, IPADDR_TYPE_V6);

	while (1) 
	{
		if (ping_send(ping_info, s, &(ping_info->addr)) == ERR_OK) 
		{
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG, ("ping6: send "));
			ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
			LWIP_DEBUGF( PING6_DEBUG, ("\n"));
			ping_mutex_unlock();
			ping_info->time = sys_now();
			ping_recv(ping_info, s);
		} 
		else 
		{
			ping_mutex_lock();
			LWIP_DEBUGF( PING6_DEBUG, ("ping: send "));
			ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
			LWIP_DEBUGF( PING6_DEBUG, (" - error\n"));
			ping_mutex_unlock();
		}

        ++ping_info->count;
		if((ping_info->target_count && (ping_info->count  ==  ping_info->target_count)) || \
			(ping_info->force_stop == true)){
			break;
		}else{
			sys_msleep( ping_info->interval);
		}
	}
	
	lwip_close(s);
	ping_statistics_report(ping_info);
	ping_list_remove(ping_info);
	ping_deinit(ping_info);

	//if(!ping_info)
	//	vPortFree(ping_info);
}

sys_thread_t
ping6_init(void *arg)
{
	char taskName[16] = {0,};	
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	sprintf(taskName, "ping6_thr_%d", ping_info->vif_id);
	return sys_thread_new(taskName, ping_thread, ping_info, LWIP_PING_TASK_STACK_SIZE, LWIP_PING_TASK_PRIORITY);
}

void
ping6_deinit(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	xTaskHandle destroy_handle = ping_info->task_handle;
	vPortFree(ping_info);//ping_info->task_handle = NULL;
	vTaskDelete(destroy_handle);
}

/////////////////////////////////////////////////////////
static int ping_recv_once(void *arg, int s)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
	char buf[64];
	int fromlen, len;
	struct sockaddr_in6 from;
	struct icmp6_echo_hdr *iecho;

	ping_info->time_delay=0;

	while((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&from, (socklen_t*)&fromlen)) > 0) 
	{
		if (len >= (int)(sizeof(struct ip6_hdr)+sizeof(struct icmp6_echo_hdr))) 
		{
			iecho = (struct icmp6_echo_hdr *)(buf + IP6_HLEN);
			if ( ( iecho->id == PING_ID ) && ( iecho->seqno == htons(ping_info->seq_num ) ) )
			{
				ping_info->time_delay = (sys_now() - ping_info->time);
				ip_addr_t fromaddr;
				inet6_addr_to_ip6addr(&fromaddr.u_addr.ip6, &from.sin6_addr);
				LWIP_DEBUGF( PING6_DEBUG, ("ping6: recv "));
				ip6_addr_debug_print_val(PING6_DEBUG, fromaddr.u_addr.ip6);
				LWIP_DEBUGF( PING6_DEBUG, (" %"U32_F" ms\n", ping_info->time_delay));
				
				/* do some ping result processing */
				PING_RESULT((iecho->type == ICMP_ER));
				ping_info->total_delay += ping_info->time_delay;
				ping_info->success++;
				return 1;
			}
		}
	}
	
	LWIP_DEBUGF( PING6_DEBUG, ("ping6: recv "));
	ip6_addr_debug_print_val(PING6_DEBUG, (ping_info->addr.u_addr.ip6));
	LWIP_DEBUGF( PING6_DEBUG, (" timeout\n"));
	
	/* do some ping result processing */
	PING_RESULT(0);
	return -1;
}

static void
ping_report(void *arg)
{
	ping_parm_t* ping_info = (ping_parm_t*)arg;
    double delay __attribute__((unused)) = (ping_info->success == 0) ? 0 : ((double)ping_info->total_delay/(double)ping_info->success);
	LWIP_DEBUGF( PING6_DEBUG, ("\n---------------------------- Ping6 Statistics --------------------------------\n"));
	LWIP_DEBUGF( PING6_DEBUG, (" Interface : wlan%d                   Target Address : ", ping_info->vif_id));
	ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
	LWIP_DEBUGF( PING6_DEBUG, ("\n"));
	LWIP_DEBUGF( PING6_DEBUG, (" %d packets transmitted, %d received, %d%% packet loss, %d.%d ms average delay",\
		 ping_info->count, ping_info->success, (ping_info->success == 0) ? 0 : 100, \
		(int)delay, (int)((delay - (int)delay) * 10)));
	LWIP_DEBUGF( PING6_DEBUG, ("\n-----------------------------------------------------------------------------\n"));
}

int ping6_once(ping_parm_t* ping_info)
{
	int ret = 0;
	int socket_tmp;
	int timeout = PING_RCV_TIMEO;

	if ((socket_tmp = lwip_socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6)) < 0) 
	{
		LWIP_DEBUGF( PING6_DEBUG, ("ping6: socket create fail\r\n"));
		return -1;
	}

	lwip_setsockopt(socket_tmp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	ping_parameters_init(ping_info);

	if (ping_send(ping_info, socket_tmp, &(ping_info->addr)) == ERR_OK) 
	{
		LWIP_DEBUGF( PING6_DEBUG, ("ping6: send "));
		ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
		LWIP_DEBUGF( PING6_DEBUG, ("\n"));
		ping_info->time = sys_now();
		ret = ping_recv_once(ping_info, socket_tmp);
	} 
	else 
	{
		LWIP_DEBUGF( PING6_DEBUG, ("ping6: send "));
		ip_addr_debug_print(PING6_DEBUG, &(ping_info->addr));
		LWIP_DEBUGF( PING6_DEBUG, (" - error\n"));
		LWIP_DEBUGF( PING6_DEBUG, ("ping6: send error"));
	}
	++ping_info->count;
	lwip_close(socket_tmp);
	ping_report(ping_info);
	return ret;
}
#endif /* LWIP_RAW */

