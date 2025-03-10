#ifndef __TCPIP_FUNC__
#define __TCPIP_FUNC__
#include "at_list.h"
#include "lwip/arch.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "timers.h"
#include "dce.h"
#include "wifi_command.h"

#define MAX_RECONN_NUM	10
#define AT_MAX_RECV_LEN  1472
#define AT_MAX_RECV_DATA_LEN	2920
#define AT_MAC_READ_DATA_LEN	2048
#define TX_BUFFER_SIZE  (4*1024)
#define MAX_TCP_SEQ_STATE_SIZE 32
#define AT_UDP_MIN_PORT				0xc000
#define AT_UDP_MAX_PORT				0xffff
typedef enum {
    conn_type_udp,
    conn_type_tcp,
    conn_type_ssl,
}conn_type_e;

typedef enum {
    conn_stat_init,
    conn_stat_connected,
    conn_stat_abort,
    conn_stat_close,
}conn_stat_e;

typedef struct {
    char             domain_str[64];
    unsigned short   src_port;
    unsigned short   dst_port;
    ip_addr_t        src_ip;
    ip_addr_t        dst_ip;
}conn_ip_info_t;

typedef struct {
    unsigned char    id;
	conn_type_e      type;
    conn_ip_info_t   ip_info;
    int              listen_fd;
    struct list_head list;
    struct list_head client_list;
}server_db_t;

typedef struct {
    unsigned char    id;
    unsigned char    close_mode;
    unsigned int     rx_timestamp;
    unsigned int     tx_timestamp;
    conn_type_e      type;
    conn_ip_info_t   ip_info;
    int              fd;
	int				 recv_win;
    conn_stat_e      state;
    server_db_t     *father; //point to the server which this client is accept by.
    struct list_head list;
    union {
        struct {
                unsigned int keep_alive;
                struct trs_tls_cfg *ssl_cfg; 
                struct trs_tls *tls;
            }tcp;
        struct {
            	// There is no special information in the current UDP, which is reserved for future expansion
            }udp;
	}priv;
	unsigned int	retry;	//reconnect times
}client_db_t;

typedef struct {
    unsigned char    buff[TX_BUFFER_SIZE];
    unsigned short   rpos;
    unsigned short   rlen;
    unsigned short   wpos;
    unsigned short   wlen;
    unsigned short   threshold;
    client_db_t      *client;
}conn_send_buff_t;

typedef struct
{
    unsigned int   read_pos;
    unsigned int   write_pos;
    unsigned int   read_len;
	unsigned int   write_len;
    unsigned char  buffer[AT_MAX_RECV_DATA_LEN];
}conn_recv_data_t;

typedef struct {
    unsigned char    buff[MAX_CONN_NUM][AT_MAX_RECV_LEN];
    conn_recv_data_t	*recv_data[MAX_CONN_NUM];
}conn_recv_buff_t;

typedef struct {
    unsigned char    server_max_conn;
    unsigned int     server_timeout;   //seconds
    unsigned char    ipmux;
    bool             remote_visible;
    unsigned char    recv_mode;
    unsigned char    pass_through;
    unsigned char    use_end_flag;
    conn_recv_buff_t recv_buff;
    conn_send_buff_t send_buff;
    unsigned char    ssl_mode;
    unsigned int     ssl_buff_size;
    TimerHandle_t    uart_rx_timeout;       //from uart to socket tx timeout.
    TimerHandle_t    reconnect_interval_times;
    QueueHandle_t    uart_rx_queue;
    unsigned char    tcp_server_id;
    unsigned char    udp_server_id;
    struct list_head server_list;
    struct list_head client_list;
}net_conn_cfg_t;

typedef int (*client_iterate_cb)(client_db_t *client, void *param_in, void *param_out);

net_conn_cfg_t * get_net_conn_cfg(void);
void at_init_net_func(dce_t* dce);
int at_net_client_start(client_db_t *client);
int at_net_client_close(client_db_t *client);
int at_net_client_prepare_tx(client_db_t *client);
client_db_t * at_net_find_client(unsigned char link_id);
client_db_t * at_net_find_client_by_status(conn_stat_e state);
int at_net_resolve_dns(const char *host, struct sockaddr_in *ip);
int at_net_server_start(server_db_t *server);
void at_net_server_stop(server_db_t *server_param);


int at_net_close(int link_id);
int at_net_abort_uart_rx(void);
void at_net_iterate_client(client_iterate_cb func, void *in, void *out);
int at_net_client_auto_start(void);

int at_net_write_buff(conn_recv_data_t *recv_data, unsigned char *src_buf, int add_len);
int at_net_read_buff(conn_recv_data_t *recv_data, unsigned char *dst_buf, int length);

#endif
