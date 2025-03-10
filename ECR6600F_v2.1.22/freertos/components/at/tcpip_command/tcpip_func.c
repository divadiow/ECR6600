
#include "trs_tls.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "cli.h"
#include "at_common.h"
#include "dce.h"
#include "tcpip_func.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"
#include "lwip/tcpbase.h"
#include "lwip/netdb.h"
#include "string.h"
#include "at_def.h"
#include "hal_flash.h"
#include "easyflash.h"
#include "system_wifi.h"
#include "basic_command.h"
#include "system_config.h"
#include "at_common.h"


#define AT_NET_TASK_STACK_SIZE   (5120)
#define AT_NET_TASK_PRIO         4

#define CONNECTSHOW_BIT (1<<1)

/*******************************quxin*************************/
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif
/*******************************quxin*************************/
typedef enum {
    NET_SEND,
    NET_CLOSE,
    NET_RECON,
}MSG_TYPE;

typedef struct rx_msg_t
{
    MSG_TYPE msg_type;
    int client_id;
}rx_msg;


typedef struct {
    const unsigned char id;
    unsigned char       used;
}link_id_pool_t;

typedef struct rx_info_t{
    struct sockaddr_in  clientAddr;
    int        rx_len;
}rx_info;

typedef enum {
    NET_NULL = (0),
    NET_CLIENT_RECV = (1 << 0),
    NET_SERVER_ACCEPT = (1 << 1),
    NET_SERVER_RECV = (1 << 2),

    NET_ERROR = (1 << 7),
}NET_POLL_RET;

// quxin static TimerHandle_t  server_hold_timer;

net_conn_cfg_t       *net_conn_cfg = NULL;
static TaskHandle_t   at_net_task_t;
static StackType_t    at_net_task_stack[AT_NET_TASK_STACK_SIZE/sizeof(StackType_t)];
static StaticTask_t   at_net_tcb;

static link_id_pool_t link_id_pool[MAX_CONN_NUM] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}};
extern u32_t ssl_client_num;

u32_t lost      = 0;
net_conn_cfg_t * get_net_conn_cfg(void)
{
    return net_conn_cfg;
}

void at_set_default_net_conn_cfg(void)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    cfg->server_max_conn = MAX_CONN_NUM;
    cfg->server_timeout = 0;
    cfg->remote_visible = false;
    cfg->ssl_buff_size = 4096;
    cfg->ssl_mode = 0;
    cfg->ipmux = 0;

	hal_system_get_config(CUSTOMER_NV_WIFI_IP_MODE, &(cfg->pass_through), sizeof(cfg->pass_through));
    //cfg->pass_through = 0;
    cfg->recv_mode = 0;
    list_init(&cfg->client_list);
    list_init(&cfg->server_list);
}

static int alloc_link_specific_id(unsigned char id)
{
    if (id >= ARRAY_SIZE(link_id_pool))
        return 1;

    if (!link_id_pool[id].used) {
        link_id_pool[id].used = 1;
        return 0;
    }

    return 1;
}

static int alloc_link_id(void)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(link_id_pool); ++i) {
        if (!link_id_pool[i].used) {
            link_id_pool[i].used = 1;
            return link_id_pool[i].id;
        }
    }

    return MAX_CONN_NUM;
}

static void free_link_id(unsigned char id)
{
    if (id >= ARRAY_SIZE(link_id_pool))
        return;
    link_id_pool[id].used = 0;
}

static int client_linkid_equal(client_db_t *client, void *link_id_param, void *target_param)
{
    unsigned char *link_id = (unsigned char *)link_id_param;
    client_db_t **target = (client_db_t **)target_param;
    if (client->id == *link_id) {
        if (target) {
            *target = client;
        }
        
        return 1;
    }

    return 0;
}

static int client_fd_equal(client_db_t *client, void *fd_param, void *target_param)
{
    int *fd = (int *)fd_param;
    client_db_t **target = (client_db_t **)target_param;
    if (client->fd == *fd) {
        if (target) {
            *target = client;
        }
        
        return 1;
    }

    return 0;
}

void at_net_iterate_client(client_iterate_cb func, void *in, void *out)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client, *client_tmp;
    server_db_t *server, *server_tmp;

    list_for_each_entry_safe(client, client_tmp, &cfg->client_list, list) {
        if (func && func(client, in, out)) {
            break;
        }
    }

    list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) {
        list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
            if (func && func(client, in, out)) {
                break;
            }
        }
    }
}

client_db_t *at_net_find_client(unsigned char link_id)
{
    client_db_t *client = NULL;

    at_net_iterate_client(client_linkid_equal, (void *)&link_id, (void *)&client);
    
    return client;
}

client_db_t *at_net_find_client_by_fd(int fd)
{
    client_db_t *client = NULL;

    at_net_iterate_client(client_fd_equal, (void *)&fd, (void *)&client);
    
    return client;
}

client_db_t *at_net_find_client_by_status(conn_stat_e state)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client, *client_tmp;
    server_db_t *server, *server_tmp;

    if (!list_empty(&cfg->client_list))
	{
	    list_for_each_entry_safe(client, client_tmp, &cfg->client_list, list) {
	        if (state == client->state) {
	            return client;
	        }
	    }
	}

	if (!list_empty(&cfg->server_list))
	{
	    list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) {
			if (!list_empty(&server->client_list))
			{
				list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
		            if (state == client->state && client->type != conn_type_udp) {
		                return client;
		            }
		        }
			}
	    }
	}

	return NULL;
}

server_db_t *at_net_find_server_by_fd(int fd)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();

    if(!list_empty(&cfg->server_list)){
		server_db_t *server_tmp;
        list_for_each_entry(server_tmp, &cfg->server_list, list) {
			if (server_tmp->listen_fd == fd)
			{
				return server_tmp;
			}
        }
    }

    return NULL;
}

int at_net_resolve_dns(const char *host, struct sockaddr_in *ip)
{
 
    struct hostent *he;
    struct in_addr **addr_list;

    he = gethostbyname(host);
    if (he == NULL) {
        return 1;
    }
    
    addr_list = (struct in_addr **)he->h_addr_list;
    if (addr_list[0] == NULL) {
        return 1;
    }
    
    ip->sin_family = AF_INET;
    memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
    
    return 0;
}

int at_net_start_tcp_udp_client(client_db_t *param)
{
    int fd, ret;
    struct sockaddr_in sock_addr, bind_addr, conn_addr;
    const int timeout = 4000;
    int type = SOCK_STREAM;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(param->ip_info.dst_port);

    if (param->ip_info.domain_str[0] != '\0') {
        if (at_net_resolve_dns(param->ip_info.domain_str, &sock_addr) < 0) {
            os_printf(LM_APP, LL_INFO, "do dns resolv failed\n");
            return 1;
        }
    } else {
        sock_addr.sin_addr.s_addr = ip_2_ip4(&param->ip_info.dst_ip)->addr;
    }

    if (param->type == conn_type_udp)
        type = SOCK_DGRAM;
    
    fd = socket(AF_INET, type, 0);
    if (fd < 0) {
        os_printf(LM_APP, LL_INFO, "create socket error[%d].", errno);
        return 1;
    }

    param->fd = fd;
    if (ip_2_ip4(&param->ip_info.src_ip)->addr || param->ip_info.src_port) {
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = htons(param->ip_info.src_port);
        bind_addr.sin_addr.s_addr = ip_2_ip4(&param->ip_info.src_ip)->addr;
        ret = bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
        if (ret) {
            close(fd);
            os_printf(LM_APP, LL_INFO, "bind failed to 0x%08x|%d, ret[%d]\n", (unsigned int)(ip_2_ip4(&param->ip_info.src_ip)->addr), 
                param->ip_info.src_port, ret);
            return 1;
        }
    }

    if (param->ip_info.src_port == 0) {
        //system_printf("%s[%d] %d\n", __FUNCTION__, __LINE__, param->ip_info.src_port);
        uint32_t len = sizeof(conn_addr);
        ret = getsockname(fd, (struct sockaddr *)&conn_addr, &len);
        if (ret == 0) {
            //system_printf("%s[%d] %d\n", __FUNCTION__, __LINE__, conn_addr.sin_port);
            param->ip_info.src_port = ntohs(conn_addr.sin_port); // 获取端口号
        }
    }

    if (type == SOCK_STREAM) {
        uint32_t TOS = 0x7;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, IPPROTO_IP, IP_TOS, &TOS, sizeof(TOS));
        if (param->priv.tcp.keep_alive) {
            int optval = 1;
            socklen_t  optlen = sizeof(optval);
            int keepIdle = param->priv.tcp.keep_alive; // 如该连接在keep_alive秒内没有任何数据往来,则进行探测 
            int keepInterval = 1; // 探测时发包的时间间隔为1 秒
            int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
            if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0 || 
               setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) < 0 ||
               setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval))< 0 ||
               setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount))< 0) {
                close(fd);
                return 1;
            }
        }
        char resp_buf[64];
        ret = connect(fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
        if (ret) {
            close(fd);
            if(cfg->ipmux == 1)
            {
                sprintf(resp_buf,"%d,%s\r\n",param->id,"CLOSED");
                target_dce_transmit(resp_buf, strlen(resp_buf));
                os_printf(LM_APP, LL_INFO, "connecting failed to 0x%08x|%d, ret[%d]\n", (unsigned int)(ip_2_ip4(&param->ip_info.dst_ip)->addr), 
                    param->ip_info.dst_port, ret);
            }
            else if(cfg->pass_through != 1)
            {
                sprintf(resp_buf,"%s\r\n","CLOSED");
                target_dce_transmit(resp_buf, strlen(resp_buf));
                os_printf(LM_APP, LL_INFO, "connecting failed to 0x%08x|%d, ret[%d]\n", (unsigned int)(ip_2_ip4(&param->ip_info.dst_ip)->addr), 
                    param->ip_info.dst_port, ret);
            }
            
            return 1;
        }
        
        //keepalive
    }
    param->state = conn_stat_connected;
    
    return 0;
}
#if 0
char dbg_ssl_cert[] = "-----BEGIN CERTIFICATE-----\r\n\
MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\r\n\
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\r\n\
PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\r\n\
KOqkqm57TH2H3eDJAkSnh6/DNKOqkqm57TH2H3eDJAkSnh6/DNKOqkqmFu0Qg==\r\n\
-----END CERTIFICATE-----\r\n";
extern char * os_strncpy(char *dest, const char *src, size_t n);
static void dbg_get_ssl_cert_pem_len(unsigned int start_addr, unsigned int *len)
{
    int i = 0;    
    char pem_buff[SSL_CERT_PEM_OTHER_LINE_LEN + 1];

    const char *ssl_pem_addr = dbg_ssl_cert;

    
    system_printf("ssl_pem_addr: %p\n", ssl_pem_addr);
    os_strncpy(pem_buff, ssl_pem_addr, SSL_CERT_PEM_FIRSET_LINE_LEN);
    pem_buff[SSL_CERT_PEM_FIRSET_LINE_LEN] = '\0';
    system_printf("%s\n", pem_buff);
    
    ssl_pem_addr = dbg_ssl_cert + SSL_CERT_PEM_FIRSET_LINE_LEN;
    system_printf("ssl_pem_addr: %p\n", ssl_pem_addr);
    
    for(i = 0; ;i++)
    {
        
        system_printf("ssl_pem_addr: %p\n", ssl_pem_addr);
        
        os_strncpy(pem_buff, ssl_pem_addr, SSL_CERT_PEM_OTHER_LINE_LEN);
        pem_buff[SSL_CERT_PEM_OTHER_LINE_LEN] = '\0';
        system_printf("%s\n", pem_buff);
        
        char *end_addr = strchr(pem_buff, '-');
        if(end_addr)
        {
            system_printf("%p\n", pem_buff);
            system_printf("%p\n", end_addr);
            system_printf("char_num: %d\n", (end_addr - pem_buff));
            
            *len = SSL_CERT_PEM_FIRSET_LINE_LEN + i * SSL_CERT_PEM_OTHER_LINE_LEN + (end_addr - pem_buff) + SSL_CERT_PEM_END_LINE_LEN;
            
            system_printf("len: %d\n", *len);
            break;
        }

        ssl_pem_addr += SSL_CERT_PEM_OTHER_LINE_LEN;
    }
}
#endif

static void get_ssl_pem_len(uint32_t start_addr, const char* start_str, const char* end_str, unsigned int *len)
{
    unsigned char *pem_buff = os_malloc(4096);
    memset(pem_buff, 0 ,4096);
    if(!pem_buff){
        os_printf(LM_APP, LL_INFO, "malloc failed %s\n",__func__);
        return;
    }
    hal_flash_read(start_addr, pem_buff, 4096);
    char* found_start = strstr((const char *)pem_buff,start_str);
    char* found_end = strstr((const char *)pem_buff,end_str);
    // os_printf(LM_APP, LL_INFO, "found_end %s\n",found_end);
    if(found_start == NULL || found_end == NULL){
        *len = 0;
    } else {
        *len = found_end - found_start + strlen(end_str);
    }
    os_free(pem_buff);
}

void at_net_free_tls_cfg(struct trs_tls_cfg *cfg)
{
    if (!cfg)
        return;
    
    if(cfg->cacert_pem_buf) {
        os_free((void *)cfg->cacert_pem_buf);
        cfg->cacert_pem_buf = NULL;
    }
    if(cfg->clientcert_pem_buf) {
        os_free((void *)cfg->clientcert_pem_buf);
        cfg->clientcert_pem_buf = NULL;
    }
    if(cfg->clientkey_pem_buf) {
        os_free((void *)cfg->clientkey_pem_buf);
        cfg->clientkey_pem_buf = NULL;
    }

    os_free(cfg);
}

extern void partion_print_all(void);
int at_net_start_ssl_client(client_db_t *param)
{
    struct trs_tls *tls = NULL;
    struct sockaddr_in sock_addr;
    char ip_str[16] = {'\0'};
    ip_addr_t ip;

	//ca CRT
    unsigned char *ssl_ca_cert_pem_buff = NULL;
    unsigned int ssl_ca_pem_len  = 0;

    // client CRT
    unsigned char *ssl_client_crt_pem_buff = NULL;
    unsigned int ssl_client_crt_pem_len    = 0;

    // client key
    unsigned char *ssl_client_key_pem_buff = NULL;
    unsigned int ssl_client_key_pem_len    = 0;

    param->priv.tcp.ssl_cfg = os_zalloc(sizeof(struct trs_tls_cfg));
    
#if 0
    dbg_get_ssl_cert_pem_len((unsigned int)dbg_ssl_cert, &ssl_pem_len);
    system_printf("ssl_pem_len: %d\n", ssl_pem_len);
	unsigned char *ssl_cert_pem_buff = (unsigned char *)os_malloc(ssl_pem_len + 1);
	system_printf("ssl_cert_pem_buff: %p\n", ssl_cert_pem_buff);
#endif

    get_ssl_pem_len(SSL_CA_PEM_FLASH_ADDR,SSL_CRT_PEM_START_STR,SSL_CRT_PEM_END_STR,&ssl_ca_pem_len);
    if(ssl_ca_pem_len){
        ssl_ca_cert_pem_buff = (unsigned char *)os_zalloc(ssl_ca_pem_len + 1);
         hal_flash_read(SSL_CA_PEM_FLASH_ADDR, ssl_ca_cert_pem_buff, ssl_ca_pem_len);
         param->priv.tcp.ssl_cfg->cacert_pem_buf = ssl_ca_cert_pem_buff;
         param->priv.tcp.ssl_cfg->cacert_pem_bytes = ssl_ca_pem_len + 1;
    }

    get_ssl_pem_len(SSL_CLIENT_CRT_PEM_FLASH_ADDR,SSL_CRT_PEM_START_STR,SSL_CRT_PEM_END_STR,&ssl_client_crt_pem_len);
    if(ssl_client_crt_pem_len){
        ssl_client_crt_pem_buff  = (unsigned char *)os_zalloc(ssl_client_crt_pem_len + 1);
        hal_flash_read(SSL_CLIENT_CRT_PEM_FLASH_ADDR, ssl_client_crt_pem_buff, ssl_client_crt_pem_len);
         param->priv.tcp.ssl_cfg->clientcert_pem_buf = ssl_client_crt_pem_buff;
         param->priv.tcp.ssl_cfg->clientcert_pem_bytes = ssl_client_crt_pem_len + 1;
    }

    get_ssl_pem_len(SSL_CLIENT_KEY_PEM_FLASH_ADDR,SSL_KEY_PEM_START_STR,SSL_KEY_PEM_END_STR,&ssl_client_key_pem_len);
    if(ssl_client_key_pem_len){
        ssl_client_key_pem_buff  = (unsigned char *)os_zalloc(ssl_client_key_pem_len + 1);
        hal_flash_read(SSL_CLIENT_KEY_PEM_FLASH_ADDR, ssl_client_key_pem_buff, ssl_client_key_pem_len);
         param->priv.tcp.ssl_cfg->clientkey_pem_buf = ssl_client_key_pem_buff;
         param->priv.tcp.ssl_cfg->clientkey_pem_bytes = ssl_client_key_pem_len + 1;
    }

    // os_printf(LM_APP, LL_INFO, "ssl_ca_pem_len %d \n",ssl_ca_pem_len);
    // os_printf(LM_APP, LL_INFO, "last %02x %02x %02x \n",ssl_ca_cert_pem_buff[ssl_ca_pem_len-1],ssl_ca_cert_pem_buff[ssl_ca_pem_len],ssl_ca_cert_pem_buff[ssl_ca_pem_len-2]);
    // os_printf(LM_APP, LL_INFO, "ssl_client_crt_pem_len %d\n",ssl_client_crt_pem_len);
    // os_printf(LM_APP, LL_INFO, "last %02x %02x %02x \n",ssl_client_crt_pem_buff[ssl_client_crt_pem_len-1],ssl_client_crt_pem_buff[ssl_client_crt_pem_len],ssl_client_crt_pem_buff[ssl_client_crt_pem_len-2]);
    // os_printf(LM_APP, LL_INFO, "ssl_client_key_pem_len %d\n",ssl_client_key_pem_len);
    // os_printf(LM_APP, LL_INFO, "last %02x %02x %02x \n",ssl_client_key_pem_buff[ssl_client_key_pem_len-1],ssl_client_key_pem_buff[ssl_client_key_pem_len],ssl_client_key_pem_buff[ssl_client_key_pem_len-2]);

    memset(&sock_addr, 0, sizeof(sock_addr));
    if (param->ip_info.domain_str[0] != '\0') {
        if (at_net_resolve_dns(param->ip_info.domain_str, &sock_addr) < 0) {
            os_printf(LM_APP, LL_INFO, "resolv domain failed...\n");
            at_net_free_tls_cfg(param->priv.tcp.ssl_cfg);
            param->priv.tcp.ssl_cfg = NULL;
            return 1;
        }
    } else {
        sock_addr.sin_addr.s_addr = ip_2_ip4(&param->ip_info.dst_ip)->addr;
    }
    
    ip_2_ip4(&ip)->addr = sock_addr.sin_addr.s_addr;
    sprintf(ip_str, "%" U16_F ".%" U16_F ".%" U16_F ".%" U16_F, ip4_addr1_16(ip_2_ip4(&ip)),
                ip4_addr2_16(ip_2_ip4(&ip)), ip4_addr3_16(ip_2_ip4(&ip)), ip4_addr4_16(ip_2_ip4(&ip)));

    tls = trs_tls_conn_new(ip_str, strlen(ip_str), param->ip_info.dst_port, param->priv.tcp.ssl_cfg);
    if (tls == NULL) {
        os_printf(LM_APP, LL_INFO, "ssl Connection failed...\n");
        at_net_free_tls_cfg(param->priv.tcp.ssl_cfg);
        param->priv.tcp.ssl_cfg = NULL;
        return 1;
    }

    param->priv.tcp.tls = tls;
    param->fd = tls->sockfd;
    param->state = conn_stat_connected;
    
    return 0;
}

int ssl_cert_dbg(cmd_tbl_t *t, int argc, char *argv[])
{    
    at_net_start_ssl_client(NULL);
    return 0;
}
//quxin SUBCMD(show, dbg_ssl, ssl_cert_dbg, " ", " ");

int at_net_client_close(client_db_t *client)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    if (conn_stat_connected == client->state) {
        if (conn_type_ssl == client->type) {
            trs_tls_conn_delete(client->priv.tcp.tls);
            at_net_free_tls_cfg(client->priv.tcp.ssl_cfg);
            client->priv.tcp.ssl_cfg = NULL;
            client->priv.tcp.tls = NULL;
            ssl_client_num = 0;
        } else {
            close(client->fd);
        }
    }

    if (client == cfg->send_buff.client) {
        cfg->send_buff.client = NULL;
        at_net_abort_uart_rx();
    }

	if (conn_type_tcp == client->type)
	{
		free(cfg->recv_buff.recv_data[client->id]);
	}
    free_link_id(client->id);
    list_del(&client->list);
    free(client);
    
    return 0;
}

static int iterate_client_close(client_db_t *client, void *link_id_param, void *para)
{
    //close by id
    unsigned char *link_id = (unsigned char *)link_id_param;
    if (client->id == *link_id) {
        at_net_client_close(client);
        return 1;
    }

    //close all
    if  (MAX_CONN_NUM == *link_id) {
        at_net_client_close(client);
    }

    return 0;
}

int at_net_client_close_by_id(unsigned char link_id)
{
    at_net_iterate_client(iterate_client_close, (void *)&link_id, NULL);
    
    return 0;
}

int at_net_client_auto_start(void)
{
    static int autonetcnn = 0;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t client = {0};
    int ret, value;
    uint8_t transLinkEn = 0;
    char content[32] = {0};
    char linktype[] = "TCP";
    char resp_buf[12] = {0};
    if (autonetcnn == 1)
        return 0;

    autonetcnn = 1;
    hal_system_get_config(CUSTOMER_NV_TRANSLINK_EN, &transLinkEn, sizeof(transLinkEn));
    if (transLinkEn != 1)
        return 0;

    memset(&client, 0, sizeof(client));
    client.type = conn_type_tcp;

    hal_system_get_config(CUSTOMER_NV_TRANSLINK_IP, &content[0], sizeof(content) - 1);
    if(!ipaddr_aton(content, &client.ip_info.dst_ip)) {
        strlcpy(client.ip_info.domain_str, content, sizeof(client.ip_info.domain_str));
    }

    hal_system_get_config(CUSTOMER_NV_TRANSLINK_PORT, &value, sizeof(value));
    client.ip_info.dst_port = value;
	hal_system_get_config(CUSTOMER_NV_TRANSLINK_TYPE, &linktype[0], sizeof(linktype) - 1);
    if (!strncmp(linktype, "UDP", 3)) {
        client.type = conn_type_udp;
        hal_system_get_config(CUSTOMER_NV_TRANSLINK_UDPPORT, &value, sizeof(value));
        client.ip_info.src_port = value;
    }
    cfg->pass_through = 1;
    ret = at_net_client_start(&client);
    if (ret) {
        cfg->pass_through = 0;
        return -1;
    }
    client_db_t * send_client = at_net_find_client(client.id);
    if(send_client){
        ret = at_net_client_prepare_tx(send_client);
         if(ret != 0){
             cfg->pass_through = 0;
            return -1;
        }
    } else {
        cfg->pass_through = 0;
        return -1;
    }
    
    sprintf(resp_buf,"%s\r\n",">");
    
    target_dce_transmit(resp_buf, strlen(resp_buf));
    
    return 0;
}

int at_net_client_start_do(client_db_t *param, bool oneshot)
{
    int ret = 1;
    char resp_buf[64] ={0};
    char *type = NULL;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    switch (param->type) {
        case conn_type_udp:
            type = "UDP";
            ret = at_net_start_tcp_udp_client(param);
            break;
        case conn_type_tcp:
            type = "TCP";
            ret = at_net_start_tcp_udp_client(param);
            break;
        case conn_type_ssl:
            type = "SSL";
            ret = at_net_start_ssl_client(param);
            break;
    }
    if(get_system_config()&CONNECTSHOW_BIT){
        //+LINK_CONN:<status_type>,<link_id>,"UDP/TCP/SSL",<c/s>,<remote_ip>,<remote_port>,<local_port>；
        sprintf(resp_buf,"%s:%d,%d,%s,%d,%s,%d,%d\r\n","+LINK_CONN",ret,param->id,type,0,
                                                                    ip4addr_ntoa(ip_2_ip4(&param->ip_info.dst_ip)),
                                                                    param->ip_info.dst_port,param->ip_info.src_port);
        target_dce_transmit(resp_buf, strlen(resp_buf));
    } else {
        if(ret == 0 && oneshot){
            if(cfg->ipmux == 1)
                sprintf(resp_buf,"%d,%s",param->id,"CONNECT");
            else
                sprintf(resp_buf,"%s","CONNECT");
            target_dce_transmit(resp_buf, strlen(resp_buf));
        }
    }
    return ret;
}

int at_net_client_start(client_db_t *param)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client;

    //link id should set before here. (user set from at cli)
    if (alloc_link_specific_id(param->id)) {
        os_printf(LM_APP, LL_INFO, "alloc link id [%d] failed\n.", param->id);
        return 1;
    }

	if (param->type == conn_type_tcp)
	{
		cfg->recv_buff.recv_data[param->id] = malloc(sizeof(conn_recv_data_t));
		if (NULL == cfg->recv_buff.recv_data[param->id])
		{
			free_link_id(param->id);
            return 1;
		}
		cfg->recv_buff.recv_data[param->id]->read_pos = 0;
		cfg->recv_buff.recv_data[param->id]->write_pos = 0;
		cfg->recv_buff.recv_data[param->id]->read_len = 0;
		cfg->recv_buff.recv_data[param->id]->write_len = AT_MAX_RECV_DATA_LEN;
	}

	// tcp recv data 2920
    if(at_net_client_start_do(param, true)) {
	if (param->type == conn_type_tcp)
		free(cfg->recv_buff.recv_data[param->id]);
        free_link_id(param->id);
        return 1;
    }

    client = malloc(sizeof(*client));
    *client = *param;
    
    list_add_tail(&client->list, &cfg->client_list);
    
    return 0;
}

int at_net_start_tcp_server(server_db_t *param, net_conn_cfg_t *cfg)
{
	int on = 1;
	struct sockaddr_in srcAddr;

	srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons(param->ip_info.src_port);
	
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (listen_sock < 0) {
		os_printf(LM_APP, LL_INFO, "Unable to create tcp socket: errno %d\n", errno);
		return -1;
	}
	int ret = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret) {
		os_printf(LM_APP, LL_INFO, "tcp failed");
		return -1;
	} else {
		os_printf(LM_APP, LL_INFO, "TCP OK");
	}

	param->listen_fd = listen_sock;
	os_printf(LM_APP, LL_INFO, "Tcp socket created fd:%d\n", param->listen_fd);

	int err = bind(listen_sock, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
	if (err != 0) {
		os_printf(LM_APP, LL_INFO, "Tcp socket unable to bind: errno %d\n", errno);
		return -1;
	}
	os_printf(LM_APP, LL_INFO, "Tcp socket binded\n");

	err = listen(listen_sock, cfg->server_max_conn);
	if (err != 0) {
		os_printf(LM_APP, LL_INFO, "Error occured during listen: errno %d\n", errno);
		return -1;
	}
	os_printf(LM_APP, LL_INFO, "Tcp socket listening\n");

	return 0;
}

int at_net_start_udp_server(server_db_t *param, net_conn_cfg_t *cfg)
{
	int on = 1;
	struct sockaddr_in srcAddr;

	srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons(param->ip_info.src_port);
	
	int listen_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (listen_sock < 0) {
		os_printf(LM_APP, LL_INFO, "Unable to create udp socket: errno %d\n", errno);
		return -1;
	}
	int ret = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret) {
		os_printf(LM_APP, LL_INFO, "udp failed");
		return -1;
	} else {
		os_printf(LM_APP, LL_INFO, "UDP OK");
	}

	param->listen_fd = listen_sock;
	os_printf(LM_APP, LL_INFO, "Udp socket created fd:%d\n", param->listen_fd);

	int err = bind(listen_sock, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
	if (err != 0) {
		os_printf(LM_APP, LL_INFO, "Udp socket unable to bind: errno %d\n", errno);
		return -1;
	}
	os_printf(LM_APP, LL_INFO, "Udp socket binded\n");

	return 0;
}

int at_net_start_ssl_server(server_db_t *param, net_conn_cfg_t *cfg)
{	
	os_printf(LM_APP, LL_INFO, "Ssl server not support\n");
	return 0;
}

int at_net_server_start_do(server_db_t *param, bool oneshot)
{
    int ret = 1;
    char resp_buf[64] ={0};
    char *type = NULL;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    switch (param->type) {
        case conn_type_udp:
            type = "UDP";
            ret = at_net_start_udp_server(param, cfg);
            break;
        case conn_type_tcp:
            type = "TCP";
            ret = at_net_start_tcp_server(param, cfg);
            break;
        case conn_type_ssl:
            type = "SSL";
            ret = at_net_start_ssl_server(param, cfg);
            break;
    }
	
    if(get_system_config() & CONNECTSHOW_BIT){
        //+LINK_CONN:<status_type>,<link_id>,"UDP/TCP/SSL",<c/s>,<remote_ip>,<remote_port>,<local_port>；
        sprintf(resp_buf,"%s:%d,%d,%s,%d,%s,%d,%d\r\n","+LINK_CONN",ret,param->id,type,0,
                                                                    ip4addr_ntoa(ip_2_ip4(&param->ip_info.dst_ip)),
                                                                    param->ip_info.dst_port,param->ip_info.src_port);
        target_dce_transmit(resp_buf, strlen(resp_buf));
    } else {
        if(ret == 0 && oneshot){
            if(cfg->ipmux == 1)
                sprintf(resp_buf,"%d,%s",param->id,"CONNECT");
            else
                sprintf(resp_buf,"%s","CONNECT");
            target_dce_transmit(resp_buf, strlen(resp_buf));
        }
    }
	
    return ret;
}

void at_net_server_stop(server_db_t *server_param)
{
    //close tcp/udp/ssl server
    net_conn_cfg_t *cfg = get_net_conn_cfg();

    if (!list_empty(&cfg->server_list)) {
       client_db_t *client, *client_tmp;
       server_db_t *server_del, *server_tmp;
       list_for_each_entry_safe(server_del, server_tmp, &cfg->server_list, list) {
           if (server_del->type == server_param->type && server_del->ip_info.src_port == server_param->ip_info.src_port) {
               unsigned char *serverid = (server_del->type == conn_type_udp) ? &cfg->udp_server_id : &cfg->tcp_server_id;
               close(server_del->listen_fd);
               list_for_each_entry_safe(client, client_tmp, &server_del->client_list, list) {
                   at_net_client_close(client);
               }

               if (server_del->type == conn_type_udp) {
                   *serverid &= ~(1 << (server_del->id - MAX_CONN_NUM));
               } else {
                   *serverid &= ~(1 << server_del->id);
               }

               list_del(&server_del->list);
               free(server_del);
           }
       }
   }
}

int at_net_server_start(server_db_t *param)
{
    server_db_t *server;
    client_db_t *client;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    int index;
	
    if(!list_empty(&cfg->server_list)){
		server_db_t *server_tmp;
        list_for_each_entry(server_tmp, &cfg->server_list, list) {
			if (server_tmp->type == param->type && server_tmp->ip_info.src_port == param->ip_info.src_port)
			{
				os_printf(LM_APP, LL_INFO, "already have server\n");
				return 0;
			}
        }
    }

    for (index = 0; index < MAX_CONN_NUM; index++) {
        unsigned char *serverid = (param->type == conn_type_udp) ? &cfg->udp_server_id : &cfg->tcp_server_id;
        if ((*serverid & (1 << index)) == 0) {
            *serverid |= (1 << index);
            break;
        }

        if (index == MAX_CONN_NUM - 1) {
            os_printf(LM_APP, LL_ERR, "server reach max conn\n");
            return 1;
        }
    }

    if (param->type == conn_type_udp)
        param->id = index + MAX_CONN_NUM;

    if(at_net_server_start_do(param, true)) {
        os_printf(LM_APP, LL_INFO, "server start error\n");
        return 1;
    }

    server = malloc(sizeof(*server));
    if (server == NULL) {
        os_printf(LM_APP, LL_ERR, "server no mem buff left\n");
        return 1;
    }

    memcpy(server, param, sizeof(*server));
    list_init(&server->client_list);
    if (server->type == conn_type_udp)
    {
        client = malloc(sizeof(*client));
        if (client == NULL) {
            free(server);
            os_printf(LM_APP, LL_ERR, "client no mem buff left\n");
            return 1;
        }
        client->id = server->id;
        client->ip_info = server->ip_info;
        client->fd = server->listen_fd;
        client->type = server->type;
        client->father = server;
        client->state = conn_stat_connected;

        list_add_tail(&client->list, &server->client_list);
    }
    list_add_tail(&server->list, &cfg->server_list);

    return 0;
}

int at_net_abort_uart_rx(void)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();

    dce_register_data_input_cb(NULL);
    target_dec_switch_input_state(COMMAND_STATE);

    memset(&cfg->send_buff, 0 ,sizeof(cfg->send_buff));
    return 0;
}

int at_net_exit_pass_through(unsigned char *data, int isr)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;
    if (data[0] == '+' && data[1] == '+'  && data[2] == '+') {

        if(conn_stat_abort == send_db->client->state) {
            send_db->client->retry = 0XFF;
        } else {
            at_net_abort_uart_rx();

            if(isr == 0){
                if (pdPASS != xTimerStop(cfg->reconnect_interval_times, 0)) {
                    os_printf(LM_APP, LL_INFO, "stop client reconnet intervel timer fialed...\n");
                }
            } else {
                if (pdPASS != xTimerStopFromISR(cfg->reconnect_interval_times, 0)) {
                   os_printf(LM_APP, LL_INFO, "stop client reconnet intervel timer fialed...\n");
                }
            }
        }

        if(isr == 0){
            if (pdPASS != xTimerStop(cfg->uart_rx_timeout, 0)) {
                os_printf(LM_APP, LL_INFO, "stop uart rx timer fialed...\n");
            }

        } else {
            if (pdPASS != xTimerStopFromISR(cfg->uart_rx_timeout, 0)) {
                os_printf(LM_APP, LL_INFO, "stop uart rx timer fialed...\n");
            }
        }

        return 1;
    }

    return 0;
}

int at_net_queue_uart_data(int isr)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rx_msg msg;
    msg.msg_type = NET_SEND;
    int ret = 0;
    if (isr) {
        ret = xQueueSendFromISR(cfg->uart_rx_queue, &msg, &xHigherPriorityTaskWoken);
    } else {
        ret = xQueueSend(cfg->uart_rx_queue, &msg, 0);
    }
    
	if(ret != pdTRUE) {
        os_printf(LM_APP, LL_INFO, "--%s: ret fail!\n", __func__);
        return 1;
	}
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return 0;
}

int at_net_queue_uart_recon(int isr, int link_id)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rx_msg msg;
    msg.msg_type = NET_RECON;
    msg.client_id = link_id;
    int ret = 0;
    if (isr) {
        ret = xQueueSendFromISR(cfg->uart_rx_queue, &msg, &xHigherPriorityTaskWoken);
    } else {
        ret = xQueueSend(cfg->uart_rx_queue, &msg, 0);
    }
    
	if(ret != pdTRUE) {
        os_printf(LM_APP, LL_INFO, "--%s: ret fail!\n", __func__);
        return 1;
	}
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return 0;
}

static void at_net_reset_send_buff(void)
{
    unsigned long flags = system_irq_save();
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;

    memset(send_db->buff, 0, sizeof(send_db->buff));
    send_db->wpos = 0;
    send_db->wlen = 0;
    send_db->rpos = 0;
    send_db->rlen = 0;
    system_irq_restore(flags);
}

int at_net_close(int link_id)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    rx_msg msg;
    msg.msg_type = NET_CLOSE;
    msg.client_id = link_id;
    int ret = xQueueSendFromISR(cfg->uart_rx_queue, &msg, &xHigherPriorityTaskWoken);
    
	if(ret != pdTRUE) {
        os_printf(LM_APP, LL_INFO, "--%s: ret fail!\n", __func__);
        return 1;
	}
    
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    return 0;
}
void client_reconnect_interval_times(TimerHandle_t xTimer)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;
    
    if (conn_stat_abort == send_db->client->state && \
        send_db->client->retry <= 10 && \
        cfg->pass_through == 1) { // re connect
        at_net_queue_uart_recon(0,send_db->client->id);
        at_net_reset_send_buff();
        send_db->client->retry++;
        os_printf(LM_APP, LL_INFO, "uart rx timeout, client reconn %d\n", send_db->client->retry);
        return;
    }
    else if (conn_stat_abort == send_db->client->state && \
        cfg->pass_through == 1) {
        char resp_buf[64] = {0};
        if(cfg->ipmux == 1)
            sprintf(resp_buf,"%d,%s",send_db->client->id,"CLOSED");
        else{
            sprintf(resp_buf,"%s","CLOSED");
        }
            
        dce_emit_extended_result_code((dce_t*)target_dce_get(), resp_buf, -1, 1);
        at_net_client_close(send_db->client);
        if ((pdPASS != xTimerStop(cfg->uart_rx_timeout, 0)) || (pdPASS != xTimerStop(cfg->reconnect_interval_times, 0))) {
            os_printf(LM_APP, LL_INFO, "stop uart rx or client reconnet intervel timer fialed...\n");
        }
        return;
    }
}


void at_net_uart_rx_timeout(TimerHandle_t xTimer)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;
    unsigned long flags = system_irq_save();
    unsigned short wt = send_db->wlen;
    unsigned short rt = send_db->rlen;
    system_irq_restore(flags);

    if (!send_db->client) {
        // os_printf(LM_APP, LL_INFO, "uart rx timeout, but client is null\n");
        return;
    }

    if (wt) {
        if(rt == 0)
        {
            unsigned long flags = system_irq_save();
            send_db->rlen += wt;
            send_db->wlen -= wt;
            system_irq_restore(flags);
        }
        at_net_queue_uart_data(0);
    }
}

//data from uart, will sendout by socket
void at_net_handle_data_from_target(void *param, const char *data, size_t size)
{
    net_conn_cfg_t   *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;    
    int i;

    if (!send_db->client) {
        os_printf(LM_APP, LL_INFO, "data handled, but client is null\n");
        return;
    }

    if (send_db->wlen + send_db->rlen >= send_db->threshold) {

        lost++;
        return;
    }

    u8_t         key[3];
    u8_t         pos     = 0;
    bool         match   = false;   
    static u8_t  key_pos = 0;

    key[0]='+';key[1]='+';key[2]='+';
    for(pos=0; pos<size; pos++)
    {
        if(key[key_pos] == data[pos])
        {
            if(key_pos >= 2)
            {
                match   = true;
                key_pos = 0;
                break;
            }
            else
            {
                key_pos++;
            }
        }
        else
        {
            key_pos = 0;
        }
    }

    if(match == true)
    {
        if (cfg->pass_through && at_net_exit_pass_through(key, 1)) {
            return;
        }            
    }

    for (i = 0; i < size && send_db->wlen + send_db->rlen < send_db->threshold; i++) {
        send_db->buff[send_db->wpos] = data[i];
#if 0
        if(cfg->use_end_flag == 1)
        {
            if(send_db->buff[send_db->wpos] == '\\' && step == 0)
            {
                step = 1;
            }
            else if (send_db->buff[send_db->wpos] == '\\' && step == 1)
            {
                step = 2;
                continue;
            }
            else if (send_db->buff[send_db->wpos] == '0' && step == 1)
            {
                flag = 1;
                send_db->wpos--;
                break;
            }
            else if (send_db->buff[send_db->wpos] == '0' && step == 2)
            {
                step = 0;
            }
            else
            {
                step = 0;
            }
        }
#endif
        send_db->wpos++;
        send_db->wpos %= TX_BUFFER_SIZE;
        send_db->wlen++;
    }


    if (send_db->wlen + send_db->rlen >= send_db->threshold) {
        send_db->rlen += send_db->wlen;
        send_db->wlen = 0;
        at_net_queue_uart_data(1);

        if (cfg->pass_through) {
            xTimerResetFromISR(cfg->uart_rx_timeout, 0);
        }
        os_printf(LM_APP, LL_INFO, "at_net_queue_uart_data \r\n");
    }
    return;
}

int at_net_client_tx()
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    conn_send_buff_t *send_db = &cfg->send_buff;
    unsigned short dataLen,data1Len;
    unsigned short out = 0;
    struct sockaddr_in sock_addr;
    char result_str[24] = {0};
    int ret = 0;
    unsigned long flags = system_irq_save();
    unsigned short txLen = send_db->rlen;
    unsigned short txPos = send_db->rpos;
    unsigned char *data = &(cfg->send_buff.buff[txPos]);
    unsigned char *data1 = cfg->send_buff.buff;
    system_irq_restore(flags);

    dataLen = TX_BUFFER_SIZE - txPos;
    dataLen = (dataLen > txLen) ? txLen : dataLen;
    data1Len = txLen - dataLen;

    if (!send_db->client || !txLen || conn_stat_connected != send_db->client->state) {
        os_printf(LM_APP, LL_INFO, "send error --%p:%d,%d\n", send_db->client, txLen, send_db->client->state);

        if(cfg->use_end_flag)
        {
            target_dce_transmit("SEND FAIL\r\n", 11);
            at_net_abort_uart_rx();
            at_net_reset_send_buff();
        }
        
        return 1;
    }
    //os_printf(LM_APP, LL_INFO, "send_db->client->type %d %d %d\n", send_db->client->type, out, txLen);

    if (conn_type_tcp == send_db->client->type) {
        while(out < dataLen) {
            ret = send(send_db->client->fd, data + out, dataLen - out, 0);
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "send request failed.[ret:%d]..\n", ret);
                break;
            }
            out += ret;
        }

        out = 0;
        while(out < data1Len) {
            ret = send(send_db->client->fd, data1 + out, data1Len - out, 0);
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "send request failed.[ret:%d]..\n", ret);
                break;
            }
            out += ret;
        }
    } else if (conn_type_udp == send_db->client->type) {
        memset(&sock_addr, 0, sizeof(sock_addr));
        sock_addr.sin_addr.s_addr = ip_2_ip4(&send_db->client->ip_info.dst_ip)->addr;
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(send_db->client->ip_info.dst_port);
        while(out < dataLen) {
            ret = sendto(send_db->client->fd, data + out, dataLen - out, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "send request failed.[ret:%d]..\n", ret);
                break;
            }
            out += ret;
        }

        out = 0;
        while(out < data1Len) {
            ret = sendto(send_db->client->fd, data1 + out, data1Len - out, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "send request failed.[ret:%d]..\n", ret);
                break;
            }
            out += ret;
        }
    } else if (conn_type_ssl == send_db->client->type) {
        while(out < dataLen) {
            ret = trs_tls_conn_write(send_db->client->priv.tcp.tls, data + out, dataLen - out);
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "tls send request failed.[ret:%d]..\n", ret);
                if (ret != MBEDTLS_ERR_SSL_WANT_READ  && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    os_printf(LM_APP, LL_INFO, "tls send, error ocurred.\n");
                }
            }
            out += ret;
        }

        out = 0;
        while(out < data1Len) {
            ret = trs_tls_conn_write(send_db->client->priv.tcp.tls, data1 + out, data1Len - out);
            if (ret < 0) {
                os_printf(LM_APP, LL_INFO, "tls send request failed.[ret:%d]..\n", ret);
                if (ret != MBEDTLS_ERR_SSL_WANT_READ  && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    os_printf(LM_APP, LL_INFO, "tls send, error ocurred.\n");
                }
            }
            out += ret;
        }
    }

    if (!cfg->pass_through) {
        if (ret >= 0) {
            sprintf(result_str, "Recv %d bytes\r\n", txLen);
            target_dce_transmit(result_str, strlen(result_str));
            target_dce_transmit("SEND OK\r\n", 9);
            send_db->client->rx_timestamp = xTaskGetTickCount();
        } else {
            target_dce_transmit("SEND FAIL\r\n", 11);
        }

        at_net_abort_uart_rx();
    } else {
        if (ret < 0) {
            if (conn_type_ssl == send_db->client->type) {
                trs_tls_conn_delete(send_db->client->priv.tcp.tls);
            } else {
                close(send_db->client->fd);
            }
            send_db->client->fd = -1;
            send_db->client->state = conn_stat_abort;
        }
        flags = system_irq_save();
        send_db->rpos = (send_db->rpos + txLen) % TX_BUFFER_SIZE;
        send_db->rlen -= txLen;
        system_irq_restore(flags);
        //os_printf(LM_APP, LL_INFO, "lost:%d,rlen:%d\n", lost, send_db->rlen);
    }

    return 0;
}

int at_net_client_prepare_tx(client_db_t *client)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();

    if (conn_stat_connected != client->state) {
        return 1;
    }

    cfg->send_buff.client = client;
	cfg->send_buff.client->retry = 0;
    if (cfg->pass_through && !cfg->ipmux) {
        cfg->send_buff.threshold = TX_BUFFER_SIZE;
    }
    cfg->send_buff.threshold = MIN(cfg->send_buff.threshold, TX_BUFFER_SIZE);
        
    dce_register_data_input_cb(at_net_handle_data_from_target);

    if (cfg->pass_through) {
        if ((pdPASS != xTimerStart(cfg->uart_rx_timeout, 0)) || (pdPASS != xTimerStart(cfg->reconnect_interval_times, 0))) {
            os_printf(LM_APP, LL_INFO, "start uart rx or client reconnect interval timer fialed...\n");
        }
    }

    target_dec_switch_input_state(TCP_ONLINE_DATA_STATE);
    
    return 0;
}

static void at_rx_show_tcp_data(dce_t* dce, rx_info* info, client_db_t *client)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    char IPD_BUF[AT_MAX_RECV_LEN] = {0};
    int pos = 0;
    char addr_str[64];
    inet_ntoa_r(((struct sockaddr_in *)&(info->clientAddr))->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);

	//active mode
    if(cfg->recv_mode == 0)
	{
        //"pos-1" bsecause of the rom code bug
        if(cfg->ipmux == 1){
            if(cfg->remote_visible == false){
                pos = sprintf(IPD_BUF,"+IPD,%d,%d:",client->id,info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%d,%s,%d:",client->id,info->rx_len, addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        } 
		else {
            if(cfg->remote_visible == false) {
                pos = sprintf(IPD_BUF,"+IPD,%d:",info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len) - 1;
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%s,%d:",info->rx_len,addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        }
        if(pos != 0)
            dce_emit_pure_response(dce, IPD_BUF, pos);
            
        dce_emit_extended_result_code(dce, (char *)&cfg->recv_buff.buff[client->id][0], info->rx_len, 1);
    }
	else 	//passive mode
	{
        // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
        int temp_len = at_net_write_buff(cfg->recv_buff.recv_data[client->id], &cfg->recv_buff.buff[client->id][0], info->rx_len);
        if ( temp_len < 0)
		{
			//config socket recv window = 0
			int recvbuff = sizeof(int);
			getsockopt(client->fd, SOL_SOCKET, SO_RCVBUF, &client->recv_win, (socklen_t*)&recvbuff);

			recvbuff = 0;
			setsockopt(client->fd, SOL_SOCKET, SO_RCVBUF, &recvbuff, sizeof(recvbuff));
		}
        
        if(cfg->ipmux == 1){
            if(temp_len > 0) {
                pos = sprintf(IPD_BUF,"+IPD,%d,%d\r\n",client->id, cfg->recv_buff.recv_data[client->id]->read_len);            
            } else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%d\r\n", client->id, 0);
            }
        }
        else {
            if(temp_len > 0) {
                pos = sprintf(IPD_BUF,"+IPD,%d\r\n", cfg->recv_buff.recv_data[client->id]->read_len);
            } else {
                pos = sprintf(IPD_BUF,"+IPD,%d\r\n", 0);
            }
        }
        if(pos != 0)
            dce_emit_pure_response(dce, IPD_BUF, pos);
        //dce_emit_pure_response(dce, (char *)&cfg->recv_buff.buff[client->id][0], info->rx_len);
    }
}

static void at_rx_show_udp_ssl_data(dce_t* dce, rx_info* info, client_db_t *client)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    char IPD_BUF[AT_MAX_RECV_LEN] = {0};
    int pos = 0;
    char addr_str[64];
    inet_ntoa_r(((struct sockaddr_in *)&(info->clientAddr))->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
	
    if(cfg->pass_through == 0){
        //"pos-1" bsecause of the rom code bug
        if(cfg->ipmux == 1){
            if(cfg->remote_visible == false){
                pos = sprintf(IPD_BUF,"+IPD,%d,%d:",client->id,info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%d,%s,%d:",client->id,info->rx_len, addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        } else {
            if(cfg->remote_visible == false) {
                pos = sprintf(IPD_BUF,"+IPD,%d:",info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len) - 1;
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%s,%d:",info->rx_len,addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        }
        if(pos != 0)
            dce_emit_pure_response(dce, IPD_BUF, pos);
            
        dce_emit_extended_result_code(dce, (char *)&cfg->recv_buff.buff[client->id][0], info->rx_len, 1);
    }else {
        // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
        dce_emit_pure_response(dce, (char *)&cfg->recv_buff.buff[client->id][0], info->rx_len);
    }
}

static void at_rx_show_client_data(dce_t* dce, rx_info* info, client_db_t *client)
{
	net_conn_cfg_t *cfg = get_net_conn_cfg();

	//only ip normal mode tcp protocol
	if (conn_type_tcp == client->type && cfg->pass_through == 0)
	{
		at_rx_show_tcp_data(dce, info, client);
    }
	else
	{
        at_rx_show_udp_ssl_data(dce, info, client);
    }
}

static void at_rx_show_server_data(dce_t* dce, rx_info* info, server_db_t *server, char *recv_buff)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    char IPD_BUF[96] = {0};
    int pos = 0;
    char addr_str[64];
    inet_ntoa_r(((struct sockaddr_in *)&(info->clientAddr))->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
	
    if(cfg->pass_through == 0)
	{
        //"pos-1" bsecause of the rom code bug
        if(cfg->ipmux == 1){
            if(cfg->remote_visible == false){
                pos = sprintf(IPD_BUF,"+IPD,%d,%d:",server->id,info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%d,%s,%d:",server->id,info->rx_len, addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        } else {
            if(cfg->remote_visible == false) {
                pos = sprintf(IPD_BUF,"+IPD,%d:",info->rx_len);
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len) - 1;
            }
            else {
                pos = sprintf(IPD_BUF,"+IPD,%d,%s,%d:",info->rx_len,addr_str, ntohs(info->clientAddr.sin_port));
                // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
            }
        }
        if(pos != 0)
            dce_emit_pure_response(dce, IPD_BUF, pos);
            
        dce_emit_extended_result_code(dce, (char *)recv_buff, info->rx_len, 1);
    }
	else 
	{
        // memcpy(IDP_BUF + pos,&cfg->recv_buff.buff[client->id][0],info->rx_len);
        dce_emit_pure_response(dce, (char *)recv_buff, info->rx_len);
    }
}

#if 0
NET_POLL_RET at_net_poll(dce_t* dce)
{
    int ret = 0;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client, *client_tmp;
    server_db_t *server, *server_tmp;
    char addr_str[64];
    int fd_num;
    struct timeval timeout;
    int fd_max = -1;
    fd_set reads;
    struct sockaddr_in clientAddr; 
    rx_info rf;
    uint32_t addrLen = sizeof(clientAddr);
    FD_ZERO(&reads);

    // only have one tcp server here
    list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) {
        FD_SET(server->listen_fd, &reads);
        fd_max = fd_max < server->listen_fd ? server->listen_fd : fd_max;
         list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
            if (conn_stat_connected == client->state) {
                FD_SET(client->fd, &reads);
                fd_max = fd_max < client->fd ? client->fd : fd_max;
            }
        }
        break;  //FIXME!!!!
    }
    
    // set local client nonblock
    list_for_each_entry_safe(client, client_tmp, &cfg->client_list, list) {
        if (conn_stat_connected == client->state) {
            FD_SET(client->fd, &reads);
            fd_max = fd_max < client->fd ? client->fd : fd_max;
        }
    }

    if (fd_max == -1)
        return NET_NULL;

    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;
    if ((fd_num = select(fd_max+1, &reads, 0, 0, &timeout)) < 0) {
        os_printf(LM_APP, LL_INFO, "server select error\n");
        return NET_ERROR;
    }
    if (fd_num == 0) {
        if (!list_empty(&cfg->server_list)){
            list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
                if(cfg->server_timeout != 0 && xTaskGetTickCount() - client->rx_timestamp >= cfg->server_timeout){
                    char resp_buf[64];
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",client->id,"CLOSED");
                    else
                        sprintf(resp_buf,"%s","CLOSED");
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                    at_net_client_close(client);
                }
            }
        }
        
        return NET_NULL;
    }

    int i = 0;
    for (i = 0; i < fd_max + 1; i++) {
        if(FD_ISSET(i, &reads)) {
            if (!list_empty(&cfg->server_list) && i == server->listen_fd) {
                int clinet_num = 0;
                
                list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
                   clinet_num++;
                }
                if(clinet_num >= cfg->server_max_conn) {
                    continue;
                }
                system_printf("accept connection:\n");
                int sock = accept(server->listen_fd, (struct sockaddr *)&clientAddr, &addrLen);
                if (sock < 0) {
                    os_printf(LM_APP, LL_INFO, "Unable to accept connection: errno %d\n", errno);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    ret |= NET_ERROR;
                    continue;
                }
                inet_ntoa_r(((struct sockaddr_in *)&clientAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
                os_printf(LM_APP, LL_INFO, "Received connection from %s:%d\n", addr_str,((struct sockaddr_in *)&clientAddr)->sin_port);

                client_db_t* client = malloc(sizeof(*client));
                client->type = conn_type_tcp;
                client->state = conn_stat_connected;
                client->fd = sock;
                client->rx_timestamp = xTaskGetTickCount();
                if (MAX_CONN_NUM == (client->id = alloc_link_id())) {
                    close(sock);
                    free(client);
                    os_printf(LM_APP, LL_INFO, "Unable to accept connection: allco link id failed\n");
                    ret |= NET_ERROR;
                    continue;
                }

                client->father = server;
                client->ip_info.src_port = clientAddr.sin_port;
                client->ip_info.src_ip.addr = clientAddr.sin_addr.s_addr;
                list_add_tail(&client->list, &server->client_list);
                char resp_buf[128];
                if(get_system_config()&CONNECTSHOW_BIT){
                    //+LINK_CONN:<status_type>,<link_id>,"UDP/TCP/SSL",<c/s>,<remote_ip>,<remote_port>,<local_port>；
                    sprintf(resp_buf,"%s:%d,%d,%s,%d,%s,%d,%d","+LINK_CONN",0,client->id,"TCP",1,addr_str,clientAddr.sin_port,server->ip_info.src_port);
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                } else {
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",client->id,"CONNECT");
                    else
                        sprintf(resp_buf,"%s","CONNECT");
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                }

                ret |= NET_SERVER_ACCEPT;
                return ret;
            } else {
                client = at_net_find_client_by_fd(i);
                if (!client) {
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    ret |= NET_ERROR;
                    continue;
                }

                // cfg->recv_buff.buff[client->id] = (uint8_t*)malloc(AT_MAX_RECV_LEN);
                // memset(cfg->recv_buff.buff[client->id], 0, AT_MAX_RECV_LEN);
                int len = 0;
                if (conn_type_ssl == client->type) {
                    
                    len = trs_tls_conn_read(client->priv.tcp.tls, &cfg->recv_buff.buff[client->id][0], AT_MAX_RECV_LEN);
                    if(len > 0){
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    while (trs_tls_get_bytes_avail(client->priv.tcp.tls) && (len > 0)) {
                        len = trs_tls_conn_read(client->priv.tcp.tls, &cfg->recv_buff.buff[client->id][0], AT_MAX_RECV_LEN);
                        if(len <= 0){
                            system_printf("ssl recv error %d\n",len);
                            break;
                        }
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    
                    if (len == MBEDTLS_ERR_SSL_WANT_WRITE  || len == MBEDTLS_ERR_SSL_WANT_READ) {
                        // free(cfg->recv_buff.buff[client->id]);
                        continue;
                    }                      
                } else {
                    len = recvfrom(i, &cfg->recv_buff.buff[client->id][0], AT_MAX_RECV_LEN, 0, (struct sockaddr*)&clientAddr,&addrLen);
                    inet_ntoa_r(((struct sockaddr_in *)&clientAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
                }
                if (len <= 0) {
                    os_printf(LM_APP, LL_INFO, "read failed, client %d,%d ret %d errno %d\n", client->fd, client->id, ret,errno);
                    char resp_buf[64];
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",client->id,"CLOSED");
                    else{
                        if(cfg->pass_through != 1)
                            sprintf(resp_buf,"%s","CLOSED");
                    }
                        
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);

                    if(client->father) {
                        at_net_client_close(client);
                    } else {
                        if (!cfg->pass_through) {
                            at_net_client_close(client);
                        } else if(cfg->pass_through && ONLINE_DATA_STATE != target_dce_get_state())
                    {
                    	at_net_client_close(client);
                    } else {
                            if (conn_type_ssl == client->type) {
                                trs_tls_conn_delete(client->priv.tcp.tls);
                            } else {
                                close(client->fd);
                            }
                            client->fd = -1;
                            client->state = conn_stat_abort;
                        }
                    }
                    ret |= NET_ERROR;
                } else {
                    client->rx_timestamp = xTaskGetTickCount();
                    if(client->type != conn_type_ssl){
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    ret |= client->father ? NET_SERVER_RECV : NET_CLIENT_RECV;                    
                }
                // free(cfg->recv_buff.buff[client->id]);
                continue;
            }
        }
    }
    return ret;
}

#else

NET_POLL_RET at_net_poll(dce_t* dce)
{
    int ret = 0;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client=NULL, *client_tmp;
    server_db_t *server=NULL, *server_tmp;
    char addr_str[64];
    int fd_num;
    struct timeval timeout;
    int fd_max = -1;
    fd_set reads;
    struct sockaddr_in clientAddr; 
    rx_info rf;
    uint32_t addrLen = sizeof(clientAddr);
    FD_ZERO(&reads);
	
    // only have one tcp server here	
    if (!list_empty(&cfg->server_list))
    {
    list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) {
        FD_SET(server->listen_fd, &reads);
        fd_max = fd_max < server->listen_fd ? server->listen_fd : fd_max;

    	if (!list_empty(&server->client_list))
    	{
        list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
            if (conn_stat_connected == client->state) {
                FD_SET(client->fd, &reads);
                fd_max = fd_max < client->fd ? client->fd : fd_max;
            }
        }
	}
        //break;
    }
    }
    // set local client nonblock
    if (!list_empty(&cfg->client_list))
    {
    list_for_each_entry_safe(client, client_tmp, &cfg->client_list, list) {
        if (conn_stat_connected == client->state) {
            FD_SET(client->fd, &reads);
            fd_max = fd_max < client->fd ? client->fd : fd_max;
        }
    }
    }

	if (fd_max == -1)
        return NET_NULL;
	
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;
    if ((fd_num = select(fd_max+1, &reads, 0, 0, &timeout)) < 0) {
        os_printf(LM_APP, LL_INFO, "server select error\n");
        return NET_ERROR;
    }
	
    if (fd_num == 0) {
        if (!list_empty(&cfg->server_list)){
            list_for_each_entry_safe(server, server_tmp, &cfg->server_list, list) {
                if (!list_empty(&server->client_list)){
                    list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
                        if (client->type == conn_type_udp)
                            continue;
                        unsigned int overflow_time = 0, current_tick = 0;
                        char resp_buf[64] = {0};
                        current_tick = xTaskGetTickCount();
                        if(current_tick < client->rx_timestamp){
                            overflow_time = (portMAX_DELAY - client->rx_timestamp) + current_tick;
                        } else{
                            overflow_time = current_tick - client->rx_timestamp;
                        }
                        if((cfg->server_timeout != 0) && (overflow_time >= cfg->server_timeout)){
                            if(cfg->ipmux == 1){
                                os_printf(LM_APP, LL_INFO, "client %d close\n", client->id);
                                sprintf(resp_buf,"%d,%s",client->id,"CLOSED");
                            } else {
                                os_printf(LM_APP, LL_INFO, "client close\n");
                                sprintf(resp_buf,"%s","CLOSED");
                            }
                            dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                            at_net_client_close(client);
                        }
                    }
                }
            }
        }
        return NET_NULL;
    }

    int i = 0;
    for (i = 0; i < fd_max + 1; i++) {
        if(FD_ISSET(i, &reads)) 
		{
			server = at_net_find_server_by_fd(i);
            if (server != NULL && i == server->listen_fd && server->type == conn_type_tcp) 
			{
                int clinet_num = 0;
                
                list_for_each_entry_safe(client, client_tmp, &server->client_list, list) {
                   clinet_num++;
                }
                if(clinet_num >= cfg->server_max_conn) {
                    continue;
                }
                system_printf("accept connection:\n");
                int sock = accept(server->listen_fd, (struct sockaddr *)&clientAddr, &addrLen);
                if (sock < 0) {
                    os_printf(LM_APP, LL_INFO, "Unable to accept connection: errno %d\n", errno);
                    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    ret |= NET_ERROR;
                    continue;
                }
                inet_ntoa_r(((struct sockaddr_in *)&clientAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
                os_printf(LM_APP, LL_INFO, "Received connection from %s:%d\n", addr_str,((struct sockaddr_in *)&clientAddr)->sin_port);

                client_db_t* client = malloc(sizeof(*client));
                client->type = conn_type_tcp;
                client->state = conn_stat_connected;
                client->fd = sock;
                client->rx_timestamp = xTaskGetTickCount();
                if (MAX_CONN_NUM == (client->id = alloc_link_id())) {
                    close(sock);
                    free(client);
                    os_printf(LM_APP, LL_INFO, "Unable to accept connection: allco link id failed\n");
                    ret |= NET_ERROR;
                    continue;
                }

				cfg->recv_buff.recv_data[client->id] = malloc(sizeof(conn_recv_data_t));
				if (NULL == cfg->recv_buff.recv_data[client->id])
				{
                    close(sock);
					free_link_id(client->id);
                    free(client);					
                    os_printf(LM_APP, LL_INFO, "Unable to accept connection: allco recv data failed\n");
                    ret |= NET_ERROR;
                    continue;
				}
				cfg->recv_buff.recv_data[client->id]->read_pos = 0;
				cfg->recv_buff.recv_data[client->id]->write_pos = 0;
				cfg->recv_buff.recv_data[client->id]->read_len = 0;
				cfg->recv_buff.recv_data[client->id]->write_len = AT_MAX_RECV_DATA_LEN;

                client->father = server;
				client->ip_info.dst_port = server->ip_info.src_port;
                client->ip_info.src_port = clientAddr.sin_port;
                ip_2_ip4(&client->ip_info.src_ip)->addr = clientAddr.sin_addr.s_addr;
                list_add_tail(&client->list, &server->client_list);
				
                char resp_buf[128] = {0};
                if(get_system_config()&CONNECTSHOW_BIT){
                    //+LINK_CONN:<status_type>,<link_id>,"UDP/TCP/SSL",<c/s>,<remote_ip>,<remote_port>,<local_port>；
                    sprintf(resp_buf,"%s:%d,%d,%s,%d,%s,%d,%d","+LINK_CONN",0,client->id,"TCP",1,addr_str,clientAddr.sin_port,server->ip_info.src_port);
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                } else {
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",client->id,"CONNECT");
                    else
                        sprintf(resp_buf,"%s","CONNECT");
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                }

                ret |= NET_SERVER_ACCEPT;
                return ret;
            }
			else if (server != NULL && i == server->listen_fd && server->type == conn_type_udp)
			{
				int len = 0;
				char recv_buff[AT_MAX_RECV_LEN];
				
				len = recvfrom(server->listen_fd, recv_buff, AT_MAX_RECV_LEN, 0, (struct sockaddr*)&clientAddr,&addrLen);
				inet_ntoa_r(((struct sockaddr_in *)&clientAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);

				if (len <= 0) 
				{
                    os_printf(LM_APP, LL_INFO, "read failed, client %d,%d ret %d errno %d\n", server->listen_fd, server->id, ret,errno);
                    char resp_buf[64] = {0};
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",server->id,"CLOSED");
                    else{
                        if(cfg->pass_through != 1)
                            sprintf(resp_buf,"%s","CLOSED");
                    }
                        
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);
                    ret |= NET_ERROR;
                    at_net_server_stop(server);
                } 
				else 
				{
                    rf.clientAddr = clientAddr;
                    rf.rx_len = len;
                    at_rx_show_server_data(dce,&rf,server,recv_buff);
                  
                    ret |= NET_SERVER_RECV;                    
                }
                continue;
			}
			else 
			{
                int read_len;
                client = at_net_find_client_by_fd(i);
                if (!client) {
                    //dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                    //ret |= NET_ERROR;
                    continue;
                }

                if (client->type == conn_type_tcp) {
                    if (cfg->recv_buff.recv_data[client->id]->write_len == 0) {
                        struct netconn *netinfo = NULL;
                        socklen_t infolen = sizeof(struct netconn *);
                        getsockopt(i, SOL_SOCKET, SO_CONNINFO, &netinfo, &infolen);
                        if (netinfo->pcb.tcp->state > ESTABLISHED) {
                            goto AT_NET_CLOSE;
                        }
                        continue;
                    }

                    read_len = (cfg->recv_buff.recv_data[client->id]->write_len > AT_MAX_RECV_LEN) ? AT_MAX_RECV_LEN :
                        cfg->recv_buff.recv_data[client->id]->write_len;
                } else {
                    read_len = AT_MAX_RECV_LEN;
                }

                int len = 0;
                if (conn_type_ssl == client->type) {
                    len = trs_tls_conn_read(client->priv.tcp.tls, &cfg->recv_buff.buff[client->id][0], read_len);
                    if(len > 0){
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    while (trs_tls_get_bytes_avail(client->priv.tcp.tls) && (len > 0)) {
                        len = trs_tls_conn_read(client->priv.tcp.tls, &cfg->recv_buff.buff[client->id][0], read_len);
                        if(len <= 0){
                            system_printf("ssl recv error %d\n",len);
                            break;
                        }
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    
                    if (len == MBEDTLS_ERR_SSL_WANT_WRITE  || len == MBEDTLS_ERR_SSL_WANT_READ) {
                        continue;
                    }                      
                } 
				else 
				{
                    len = recvfrom(i, &cfg->recv_buff.buff[client->id][0], read_len, 0, (struct sockaddr*)&clientAddr,&addrLen);
                    inet_ntoa_r(((struct sockaddr_in *)&clientAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str)-1);
                }
				
                if (len <= 0) 
				{
                    os_printf(LM_APP, LL_INFO, "read failed, client %d,%d ret %d errno %d\n", client->fd, client->id, ret,errno);
                    char resp_buf[64] = {0};
AT_NET_CLOSE:
                    if(cfg->ipmux == 1)
                        sprintf(resp_buf,"%d,%s",client->id,"CLOSED");
                    else{
                        sprintf(resp_buf,"%s","CLOSED");
                    }
                        
                    dce_emit_extended_result_code(dce, resp_buf, -1, 1);

                    if(client->father && client->type != conn_type_udp) {
                        at_net_client_close(client);
                    } else if(cfg->pass_through && TCP_ONLINE_DATA_STATE != target_dce_get_state())
                    {
                    	at_net_client_close(client);
                    } else {
                        if (!cfg->pass_through) {
                            at_net_client_close(client);
                        } else {
                            if (conn_type_ssl == client->type) {
                                trs_tls_conn_delete(client->priv.tcp.tls);
                            } else {
                                close(client->fd);
                            }
                            client->fd = -1;
                            client->state = conn_stat_abort;
                        }
                    }
                    ret |= NET_ERROR;
                } 
				else 
				{
                    client->rx_timestamp = xTaskGetTickCount();
                    if(client->type != conn_type_ssl){
                        rf.clientAddr = clientAddr;
                        rf.rx_len = len;
                        at_rx_show_client_data(dce,&rf,client);
                    }
                    ret |= client->father ? NET_SERVER_RECV : NET_CLIENT_RECV;                    
                }
                continue;
            }
        }
    }
    return ret;
}

#endif

static void at_net_task(void *pvParams)
{
    int  ret = 0;
    rx_msg dummy;
    dce_t* dce = (dce_t*) pvParams;
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    while (1) {
        ret = at_net_poll(dce);
        if(ret & NET_SERVER_ACCEPT){
            //todo
        } else if(ret & NET_SERVER_RECV){
            //todo
        } else if(ret == NET_NULL){
            //todo
        }

        if (pdPASS == xQueueReceive(cfg->uart_rx_queue, &dummy, 0)) {
            os_printf(LM_APP, LL_DBG, "msg_type %d\n",dummy.msg_type);
            if(dummy.msg_type == NET_CLOSE){
                at_net_client_close_by_id(dummy.client_id);
                ret |= 1;
            } else if(dummy.msg_type == NET_RECON){
                client_db_t * client = at_net_find_client(dummy.client_id);
                if(client) {
                    if(at_net_client_start_do(client, true)) {
                        ret |= 1;
                    }
					else {
						client->retry = 0;
					}
                }
            }
            //data handled, sendout it by socket.NET_SEND
            else
            {
                at_net_client_tx();
                xQueueReset(cfg->uart_rx_queue);
                ret |= 1;
            }
        }
                
        if (!ret) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void at_net_task_init(dce_t* dce)
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    
    cfg->uart_rx_queue = xQueueCreate(50, sizeof(rx_msg));
    cfg->uart_rx_timeout = xTimerCreate("uart_rx_timeout", pdMS_TO_TICKS(80), pdTRUE, (void *)0, at_net_uart_rx_timeout);
    cfg->reconnect_interval_times = xTimerCreate("reconnect_interval_times", pdMS_TO_TICKS(5000), pdTRUE, (void *)0, client_reconnect_interval_times);
    at_net_task_t = xTaskCreateStatic(at_net_task,
                "at_net", AT_NET_TASK_STACK_SIZE/sizeof(StackType_t),
                dce, AT_NET_TASK_PRIO, &at_net_task_stack[0], &at_net_tcb);
    
    if (!at_net_task_t) {
        os_printf(LM_APP, LL_INFO, "[%s, %d] task creation failed!\n", __func__, __LINE__);
    }
}

void at_init_net_func(dce_t* dce)
{
    net_conn_cfg = malloc(sizeof(*net_conn_cfg));
    if (!net_conn_cfg) {
        os_printf(LM_APP, LL_INFO, "alert!!! malloc fail in %s\n", __func__);
    }
    memset(net_conn_cfg, 0, sizeof(*net_conn_cfg));
    at_set_default_net_conn_cfg();
    at_net_task_init(dce);
}

#if 0 //quxin
static int at_net_debug(cmd_tbl_t *t, int argc, char *argv[])
{
    net_conn_cfg_t *cfg = get_net_conn_cfg();
    client_db_t *client;
    server_db_t *server;

    os_printf(LM_APP, LL_INFO, "\nipmux:%d, passthrouth:%d, max:%d, ssl_buf:%d, ssl_mode:%d, recv_mode:%d\n",
        cfg->ipmux, cfg->pass_through, cfg->server_max_conn,
        cfg->ssl_buff_size, cfg->ssl_mode, cfg->recv_mode);
    
    if (!list_empty(&cfg->client_list)) {
        list_for_each_entry(client, &cfg->client_list, list) {
            os_printf(LM_APP, LL_INFO, "client-- id:%d, fd:%d, state:%d, type:%d, sip:%08x, sport:%d, dip:%d, dport:%d\n",
                client->id, client->fd, client->state, client->type, client->ip_info.src_ip.addr, client->ip_info.src_port,
                client->ip_info.dst_ip.addr, client->ip_info.dst_port);
        }
    }

    if (!list_empty(&cfg->server_list)) {
        list_for_each_entry(server, &cfg->server_list, list) {
            os_printf(LM_APP, LL_INFO, "server-- sip:%08x, sport:%d, dip:%d, dport:%d\n",
                    server->ip_info.src_ip.addr, server->ip_info.src_port,
                    server->ip_info.dst_ip.addr, server->ip_info.dst_port);
            list_for_each_entry(client, &server->client_list, list) {
                os_printf(LM_APP, LL_INFO, "client-- id:%d, fd:%d, state:%d, type:%d, sip:%08x, sport:%d, dip:%d, dport:%d\n",
                    client->id, client->fd, client->state, client->type, client->ip_info.src_ip.addr, client->ip_info.src_port,
                    client->ip_info.dst_ip.addr, client->ip_info.dst_port);
            }
        }
    }
    
	return CMD_RET_SUCCESS;
}
#endif

int at_net_write_buff(conn_recv_data_t *recv_data, unsigned char *src_buf, int add_len)
{
    if (add_len > AT_MAX_RECV_DATA_LEN)
        return -1;

    if (add_len > recv_data->write_len)
        add_len = recv_data->write_len;

    if (recv_data->write_pos + add_len >= AT_MAX_RECV_DATA_LEN)
    {
        int len1 = AT_MAX_RECV_DATA_LEN - recv_data->write_pos;
        int len2 = add_len - len1;
        if (len1 > 0)
        {
            memcpy(&recv_data->buffer[recv_data->write_pos], src_buf, len1);
        }

        if (len2 > 0)
        {
            memcpy(&recv_data->buffer[0], src_buf + len1, len2);
        }
        recv_data->write_pos = len2;
    }
    else
    {
        memcpy( &recv_data->buffer[recv_data->write_pos], src_buf, add_len);
        recv_data->write_pos += add_len;
    }
    recv_data->write_len -= add_len;

    if (recv_data->read_len + add_len > AT_MAX_RECV_DATA_LEN)
    {
        recv_data->read_len = AT_MAX_RECV_DATA_LEN;
    }
    else
    {
        recv_data->read_len += add_len;
    }

    return add_len;
}

int at_net_read_buff(conn_recv_data_t *recv_data, unsigned char *dst_buf, int length)
{
    int act_len = length >= recv_data->read_len ? recv_data->read_len : length;

    if(act_len <= 0 || length > AT_MAC_READ_DATA_LEN)
        return -1;

    if (AT_MAX_RECV_DATA_LEN - recv_data->read_pos >= act_len)
    {
        memcpy(dst_buf, &recv_data->buffer[recv_data->read_pos], act_len);
        recv_data->read_pos += act_len;
    }
    else
    {
        int len1 = AT_MAX_RECV_DATA_LEN - recv_data->read_pos;
        int len2 = act_len - len1;
        if (len1 > 0)
        {
            memcpy(dst_buf, &recv_data->buffer[recv_data->read_pos], len1);
        }

        if (len2 > 0)
        {
            memcpy(dst_buf + len1, &recv_data->buffer[0], len2);
        }
        recv_data->read_pos = len2;
    }
    recv_data->read_len -= act_len;

    if(recv_data->write_len + act_len > AT_MAX_RECV_DATA_LEN)
    {
        recv_data->write_len = AT_MAX_RECV_DATA_LEN;
    }
    else
    {
        recv_data->write_len += act_len;
    }

    return act_len;
}

// quxin CMD(at_net, at_net_debug, "at_net", "at_net");
