/**
 * @file http_ota.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-4-15
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "system_def.h"
#include "oshal.h"
//#include "os.h"
#include "flash.h"
#include "easyflash.h"

#include "http_ota.h"
/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/
http_instance_t g_http_instance;

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/

http_instance_t *get_http_instance(void)
{
    return &g_http_instance;
}

int http_get_port_from_url(char *url)
{
    int port = HTTP_DEFAULT_PORT;

    if (url == 0)
    {
        return port;
    }

    char *path_abs = strstr(url, "http://");
    if (path_abs)
    {
        path_abs = path_abs + strlen("http://");
    }
    path_abs = os_strdup(path_abs);

    char *port_index = strchr(path_abs, '/');
    if (port_index)
    {
        *port_index = 0x00;
    }

    port_index = strchr(path_abs, ':');
    if (port_index)
    {
        port = atoi(port_index + 1);
    }

    os_free(path_abs);

    return port;
}

char *http_get_host_from_url(char *url)
{
    int i = 0;
    int len  = 0;
    char *host = NULL;
    char *pAddr = NULL;

    if (url == NULL)
    {
        return NULL;
    }
    pAddr = strstr(url, "http://");

    if(pAddr)
        pAddr = url + strlen("http://");
    else
        pAddr = url;
    len = strlen(pAddr);

    for (i = 0; i < len; i++)
    {
        if (pAddr[i] == '/' || pAddr[i] == ':')
        {
            break;
        }
    }

    host = os_malloc(i + 2);
    memset(host, 0, i + 2);
    memcpy(host, pAddr, i);

    return host;
}

static void http_client_ctx_destroy()
{
    http_instance_t *httpinst = get_http_instance();

    if (httpinst->http_ctx == NULL)
    {
        return;
    }

    if (httpinst->http_ctx->host != NULL)
    {
        os_free(httpinst->http_ctx->host);
        httpinst->http_ctx->host = NULL;
    }

    if (httpinst->http_ctx->http_header_buff != NULL)
    {
        os_free(httpinst->http_ctx->http_header_buff);
        httpinst->http_ctx->http_header_buff = NULL;
    }

    if (httpinst->http_ctx->http_rcv_buff != NULL)
    {
        os_free(httpinst->http_ctx->http_rcv_buff);
        httpinst->http_ctx->http_rcv_buff = NULL;
    }

    if (httpinst->http_ctx->socket != -1)
    {
        close(httpinst->http_ctx->socket);
        httpinst->http_ctx->socket = -1;
    }

    os_free(httpinst->http_ctx);
    httpinst->http_ctx = NULL;
}

static int http_client_ctx_create()
{
    http_instance_t *httpinst = get_http_instance();

    if (httpinst->http_ctx)
    {
        http_client_ctx_destroy();
    }

    httpinst->http_ctx = (http_client_ctx_t *)os_malloc(sizeof(http_client_ctx_t));
    if (httpinst->http_ctx == NULL)
    {
        os_printf(LM_APP, LL_ERR, "create http client ctx fail\n");
        return -1;
    }
    memset(httpinst->http_ctx, 0, sizeof(http_client_ctx_t));
    httpinst->http_ctx->socket = -1;

    httpinst->http_ctx->port = http_get_port_from_url(httpinst->url);
    httpinst->http_ctx->host = http_get_host_from_url(httpinst->url);
    httpinst->http_ctx->path = strstr(httpinst->url, httpinst->http_ctx->host) + strlen(httpinst->http_ctx->host);
    httpinst->http_ctx->path = strchr(httpinst->http_ctx->path,'/');
    os_printf(LM_APP, LL_DBG, "host:[%s] port:[%d] path:[%s]\n", httpinst->http_ctx->host, httpinst->http_ctx->port, httpinst->http_ctx->path);

    httpinst->http_ctx->http_header_buff = os_malloc(HTTP_DEFAULT_HEADER_ELN);
    if (httpinst->http_ctx->http_header_buff == NULL)
    {
        http_client_ctx_destroy();
        os_printf(LM_APP, LL_ERR, "http client header buff malloc fail\n");
        return -1;
    }

    httpinst->http_ctx->http_header_count = 0;
    httpinst->http_ctx->http_content_length = 0;
    httpinst->http_ctx->rcvd_len = 0;
    httpinst->http_ctx->buff_len = HTTP_DEFAULT_DOWNLOAD_LEN;
    httpinst->http_ctx->http_rcv_buff = os_malloc(httpinst->http_ctx->buff_len);
    if (httpinst->http_ctx->http_rcv_buff == NULL)
    {
        http_client_ctx_destroy();
        os_printf(LM_APP, LL_ERR, "http recv buffer malloc failed\n");
        return -1;
    }
	os_printf(LM_APP, LL_DBG, "return http_client_ctx_create\n");
    return 0;
}

static void http_client_instance_reset(void)
{
    http_instance_t *httpinst = get_http_instance();

    httpinst->update_status = OTA_UPDATE_NONE;
    httpinst->download_status = OTA_DOWNLOAD_IDLE;
    httpinst->error_status = OTA_ERROR_NONE;

    httpinst->file_size = 0;
    httpinst->receive_len = 0;
    httpinst->first_header = 0;

    http_client_ctx_destroy();

    os_printf(LM_APP, LL_DBG, "http instance reset success\n");
}

void http_client_instance_init(void)
{
    http_instance_t *httpinst = get_http_instance();

    httpinst->ota_init = 0;
    memset(httpinst->url, 0, sizeof(httpinst->url));

    httpinst->update_status = OTA_UPDATE_NONE;
    httpinst->download_status = OTA_DOWNLOAD_IDLE;
    httpinst->error_status = OTA_ERROR_NONE;

    httpinst->file_size = 0;
    httpinst->receive_len = 0;
    httpinst->first_header = 0;

    httpinst->http_ctx = NULL;

    httpinst->ota_dowload_time = 5;

    os_printf(LM_APP, LL_DBG, "http instance init success\r\n");
}

/* 从ota升级命令的url中解析出host port ip，动态创建http_ctx，create http server socket，connect socket */
static int http_client_connect_server(void)
{
    http_instance_t *httpinst = get_http_instance();
    os_printf(LM_APP, LL_DBG, "httpinst url=%s\n", httpinst->url);

	
    http_client_ctx_t *httpctx = NULL;
    struct hostent *host_ip_info = NULL;
    int timeout = HTTP_DEFAULT_TIMEOUT;
    struct sockaddr_in http_server_addr;

    /* 收到ota命令后，解析host，port，动态创建处理HTTP协议需要的的ctx */
    if (http_client_ctx_create() != 0)
    {
        return -1;
    }

    httpctx = httpinst->http_ctx;

    /* 根据HTTP的域名获取HTTP服务器IP*/
    host_ip_info = gethostbyname(httpctx->host);
	//os_printf(LM_APP, LL_INFO, "- - - - - - - - - httpctx->host=%s\n", httpctx->host);
    if (host_ip_info == NULL)
    {
        os_printf(LM_APP, LL_ERR, "gethostbyname %s failed!!\n", httpctx->host);
        return -1;
    }
    memcpy(&httpctx->http_server_ip, host_ip_info->h_addr_list[0], host_ip_info->h_length);
    os_printf(LM_APP, LL_INFO, "get http server IP is:%s\n", inet_ntoa(httpctx->http_server_ip));

    httpctx->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (httpctx->socket == -1)
    {
        os_printf(LM_APP, LL_ERR, "create http socket fail\n");
        return -1;
    }

    setsockopt(httpctx->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    http_server_addr.sin_family = AF_INET;
	#ifdef CONFIG_IPV6
	http_server_addr.sin_addr.s_addr = httpctx->http_server_ip.u_addr.ip4.addr;
	#else
    http_server_addr.sin_addr.s_addr = httpctx->http_server_ip.addr;
	#endif
    http_server_addr.sin_port = htons(httpctx->port);

    /* 连接socket */
	//os_printf(LM_APP, LL_INFO, "- - - - - - - - - socket=%d  addr=0x%08x  port=%d\n", httpctx->socket, http_server_addr.sin_addr.s_addr, http_server_addr.sin_port);
    if (connect(httpctx->socket, (struct sockaddr*)&http_server_addr, sizeof(http_server_addr)) < 0)
    {
        close(httpctx->socket);
        httpctx->socket = -1;
        os_printf(LM_APP, LL_ERR, "connect http server Fail!!\n");

        return -1;
    }

    return 0;
}

static int http_client_data_handle(unsigned char *data, int len)
{
    http_instance_t *httpinst = get_http_instance();
    http_client_ctx_t *httpctx = httpinst->http_ctx;
    unsigned char *data_pos = NULL;       /* 保存此次从socket获取的数据，去掉HTTP头之后的起始地址 */
    unsigned short data_len = 0;          /* 保存此次从socket获取的数据, 去掉HTTP头之后的长度 */
    unsigned short http_header_len = 0;   /* 保存此次http下载，收到的http响应行和响应头的长度 */
    unsigned short i = 0;

    if (httpinst->download_status == OTA_DOWNLOAD_IDLE)
    {
        httpinst->download_status = OTA_DOWNLOAD_ING;
    }
	

    if (httpctx->crlf_num < 4)
    {
        for (i = 0; i < len; i++)
        {
            httpctx->http_header_buff[httpctx->http_header_count++] = data[i];
            httpctx->http_header_buff[httpctx->http_header_count] = '\0';

            /* http的响应头最后结束的标识是：\r\n\r\n，保证是连续的4个 */
            if (data[i] == '\r' || data[i] == '\n')
            {
                httpctx->crlf_num++;
            }
            else
            {
                httpctx->crlf_num = 0;
            }

            /* http的头读取完毕 */
            if (httpctx->crlf_num == 4)
            {
                os_printf(LM_APP, LL_DBG, "\r\n# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #\r\n");
//                os_printf(LM_APP, LL_INFO, "recv http header len: %d, content is following:\r\n%s", 
//					httpctx->http_header_count, 
//					httpctx->http_header_buff);
				os_printf(LM_APP, LL_DBG, "recv http header len: %d, content is following:\r\n", httpctx->http_header_count);
				
				char *temp_buff = os_malloc(101);
				int temp_len = httpctx->http_header_count;
				memset(temp_buff, 0, 101);
				os_printf(LM_APP, LL_DBG, "req_data =");
				for(int temp_i=0; temp_i<httpctx->http_header_count; temp_i+=100, temp_len-=100)
				{
					memcpy(temp_buff, httpctx->http_header_buff+temp_i, MIN(100, temp_len));
					os_printf(LM_APP, LL_DBG, "%.*s", MIN(100, temp_len), temp_buff);
				}
				os_printf(LM_APP, LL_DBG, "\r\n");
				os_free(temp_buff);
                os_printf(LM_APP, LL_DBG, "\r\n# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #\r\n");

				
				if (strstr((char *)httpctx->http_header_buff, "200 OK") != NULL)
				{
	                char *content_string = strstr((char *)httpctx->http_header_buff, "Content-Length: ");
	                if (content_string)
	                {
	                    char *content_length_value_string = content_string + strlen("Content-Length: ");
	                    char len_str[20] = {0};
	                    char *p = strchr(content_length_value_string, '\r');
	                    memcpy(len_str, content_length_value_string, p - content_length_value_string);
	                    httpctx->http_content_length = atoi(len_str);
	                    httpctx->rcvd_len = 0;

	                    /** 由于利用的rcvd_total_len判断的断点续传和是否是下载完成，所以只有在第一次收到HTTP响应时才设置为0，断点续传时不能再重置为0 */
	                    if (httpinst->first_header == 0)
	                    {
	                        httpinst->receive_len = 0;
	                        httpinst->first_header = 1;
	                    }
	                }

	                content_string = strstr((char *)httpctx->http_header_buff, "Content-Range:");
	                if (content_string == NULL)
	                {
	                    /** 服务器不支持断电续传，则每次下载需要初始化 */
	                    httpinst->ota_init = 0;
	                }
			else
			{
                            httpinst->ota_init = 1;
			}

	                http_header_len = i + 1;
	                data_pos = data + http_header_len;
	                data_len = (unsigned short)(len - http_header_len);
	                break;
				}else
				{
					os_printf(LM_APP, LL_ERR, "download error\r\n");
					return 2;
				}
            }
        }
    }

    /* 此次socket接收没有收全http的响应头 */
    if (httpctx->crlf_num < 4)
    {
        os_printf(LM_APP, LL_DBG, "this time socket rcv, http header is not complete, wait netx socket read \r\n");
        return 0;
    }

    /* 根据下载过程中的首次HTTP响应中指定的Content-Length，记录版本文件的大小 */
    if (httpctx->http_content_length != 0)
    {
        if (httpinst->file_size == 0)
        {
            httpinst->file_size = httpctx->http_content_length;
            os_printf(LM_APP, LL_INFO, "download file_size: %d\n", httpinst->file_size);
        }

        if (httpinst->file_size < sizeof(ota_package_head_t)+1)
        {
            os_printf(LM_APP, LL_INFO, "download file_size:%d less than head_size\n", httpinst->file_size);
            return -1;
        }

        /* http_header_len 为0时，说明此次socket read到的都是data */
        if (http_header_len == 0)
        {
            data_pos = data;
            data_len = len;
        }

        if (data_len == 0)
        {
            os_printf(LM_APP, LL_INFO, "this time only rcvd http header\n");
            return 0;
        }

        /* 写入flash */
        if (ota_write(data_pos, data_len) != 0)
        {
            httpinst->error_status = OTA_ERROR_WRITE_FAIL;
            httpinst->ota_init = 0;
            return -1;
        }

        /* hal_write完成之后，再更新rcvd_total_len */
        httpctx->rcvd_len += data_len;
        httpinst->receive_len += data_len;
    }

    /* 此次HTTP连接指定的数据已经获取完毕 */
    if (httpinst->receive_len >= httpinst->file_size)
    {
        httpinst->download_status = OTA_DOWNLOAD_COMPLETE;
        os_printf(LM_APP, LL_INFO, "\nhttp_rcvd_len: %d rcvd_total_len %d, download ver ok\n", httpctx->rcvd_len, httpinst->receive_len);
    }

    return 0;
}

int http_client_download_request(void)
{
    http_instance_t *httpinst = get_http_instance();
    http_client_ctx_t *httpctx = NULL;
    char req_data[HTTP_DEFAULT_REQ_LEN] = {0};

    // 解析url，并连接Http server
    if (http_client_connect_server() != 0)
    {
        os_printf(LM_APP, LL_ERR, "ota connect to http server fail\n");
        httpinst->error_status = OTA_ERROR_HTTP_START_FAIL;
        return -1;
    }

    httpctx = httpinst->http_ctx;
    sprintf(req_data + strlen(req_data), "GET %s HTTP/1.1\r\n", httpctx->path);
    if (httpctx->port != 80)
    {
        sprintf(req_data + strlen(req_data), "Host: %s:%d\r\n", httpctx->host, httpctx->port);
    }
    else
    {
        sprintf(req_data + strlen(req_data), "Host: %s\r\n", httpctx->host);
    }
    sprintf(req_data + strlen(req_data), "%s", "Connection: Keep-Alive\r\n");
    sprintf(req_data + strlen(req_data), "%s", "Content-Type: application/octet-stream\r\n");  // octet-stream  application/x-www-form-urlencoded
    //sprintf(req_data + strlen(req_data), "Range: bytes=%d-\r\n", httpinst->receive_len);
    sprintf(req_data + strlen(req_data), "\r\n");
    //os_printf(LM_APP, LL_INFO, "req_data:%s#####\n", req_data);
    
	int temp_len = strlen(req_data);
	char *temp_buff = os_malloc(101);
	memset(temp_buff, 0, 101);
    os_printf(LM_APP, LL_DBG, "req_data =");
	for(int temp_i=0; temp_i<strlen(req_data); temp_i+=100, temp_len-=100)
	{
		memcpy(temp_buff, req_data+temp_i, MIN(100, temp_len));
		os_printf(LM_APP, LL_DBG, "%.*s", MIN(100, temp_len), temp_buff);
	}
	os_printf(LM_APP, LL_DBG, "\r\n");
	os_free(temp_buff);

    if (send(httpctx->socket, req_data, strlen(req_data), 0) < 0)
    {
        if (httpctx->socket != -1)
        {
            close(httpctx->socket);
            httpctx->socket = -1;
            os_printf(LM_APP, LL_DBG, "socket send fail\n");
            /* send失败，说明网络不好，close socket，设置err_code为send_fail，然后返回，在外层进行断点续传处理 */
            httpinst->error_status = OTA_ERROR_SOCKET_SEND_FAIL;
        }

        os_printf(LM_APP, LL_DBG, "ota send http request fail!\n");
        return -1;
    }

    return 0;
}

int http_client_download_file(const char *url, unsigned int len)
{
    http_instance_t *httpinst = get_http_instance();
    http_client_ctx_t *httpctx = NULL;
    int recv_len;

	memcpy(httpinst->url, url, len);

    if (httpinst->ota_init == 0)
    {
        if (ota_init() != 0)
        {
            httpinst->error_status = OTA_ERROR_INIT_FAIL;
            return -1;
        }
        http_client_instance_reset();
    }

    if (http_client_download_request() != 0)
    {
        return -1;
    }

    httpctx = httpinst->http_ctx;
    while (httpinst->download_status != OTA_DOWNLOAD_COMPLETE)
    {
        recv_len = recv(httpctx->socket, httpctx->http_rcv_buff, httpctx->buff_len, 0);
        //os_printf(LM_APP, LL_INFO, "socket recv %d data from server\n", recv_len);
        if (recv_len <= 0)
        {
            httpinst->error_status = OTA_ERROR_SOCKET_RECEIVE_FAIL;
            httpinst->ota_dowload_time--;
            http_client_ctx_destroy();
            return -1;
        }

        if (http_client_data_handle(httpctx->http_rcv_buff, recv_len) != 0)
        {
            http_client_ctx_destroy();
            return -1;
        }
    }

    http_client_ctx_destroy();
    if (httpinst->download_status != OTA_DOWNLOAD_COMPLETE)
    {
        httpinst->update_status = OTA_UPDATE_FAIL;
        return -1;
    }

    httpinst->update_status = OTA_UPDATE_SUCCESS;
    return 0;
}


//int ota_version_register(const char *version)
//{
//    ota_info_t *ota_info = get_ota_info();
//    int i, j = 0;
//
//    memset(ota_info->version, 0, sizeof(ota_info->version));
//    for (i = 0; i < strlen(version) && i < OTA_VERSION_NUM; i++)
//    {
//        if (version[i] == '.')
//        {
//            continue;
//        }
//
//        if ((version[i] < 0x30) || (version[i] > 0x39))
//        {
//            return -1;
//        }
//
//        ota_info->version[OTA_VERSION_NUM - ++j] = version[i] - 0x30;
//    }
//
//    return 0;
//}



