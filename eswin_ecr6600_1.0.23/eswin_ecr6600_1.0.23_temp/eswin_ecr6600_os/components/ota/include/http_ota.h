/**
 * @file filename
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


#ifndef _HTTP_OTA_H
#define _HTTP_OTA_H


/*--------------------------------------------------------------------------
* 	                                        	Include files
--------------------------------------------------------------------------*/
#include "ota.h"

/*--------------------------------------------------------------------------
* 	                                        	Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

#define HTTP_DEFAULT_REQ_LEN		2048
#define HTTP_DEFAULT_PORT         	80
#define HTTP_DEFAULT_HEADER_ELN   	1024
#define HTTP_DEFAULT_DOWNLOAD_LEN 	2048
#define HTTP_DEFAULT_TIMEOUT      	10000


/*--------------------------------------------------------------------------
* 	                                        	Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */
typedef struct
{
    int             port;
    char            *host;                  // malloc动态申请内存，保存ota下载地址的的域名
    char            *path;                  // 从ota upgrade_param的url里截取的字串
    ip_addr_t       http_server_ip;         // 解析host之后得到的http server的IP地址
    int             socket;                 // 进行HTTP传输的socket
    unsigned char   crlf_num;               // http以crlf(\r\n)结束一行，连续的四个/r or /n代表了http header的结束，因为请求头和请求体之间隔了一行
    char            *http_header_buff;      // malloc动态申请内存，保存从socket读取出来的HTTP的头
    unsigned short  http_header_count;      // 记录HTTP头的长度
    unsigned int    http_content_length;    // HTTP中Content-Length字段，记录本次HTTP传输要发送的数据总量
    unsigned int    rcvd_len;               // 此次http连接已获取到的数据总量
    unsigned short  buff_len;               // 设定每次从http socket获取数据的大小
    unsigned char   *http_rcv_buff;         // malloc动态申请内存，保存从http socket获取的数据
} http_client_ctx_t;

typedef struct
{
    char 				url[OTA_DEFAULT_BUFF_LEN];   		// 传入的URL地址
    unsigned char       ota_init;           // 防止断点续传时，再次初始化ota
    ota_update_status   update_status;      // OTA升级结果
    ota_download_status download_status;    // OTA是否开始下载，下载是否完成
    ota_error_status    error_status;
    unsigned int        file_size;          // 首次发起HTTP下载时，server返回的HTTP响应Content-Length字段，记录了完整的版本的文件的长度
    unsigned int        receive_len;        // 升级过程中获取数据总量
    unsigned char       first_header;       // 只有在第一次收到HTTP连接请求时，才设置receive_len，断点续传时不设置
    http_client_ctx_t   *http_ctx;          // malloc动态申请内存，保存处理http下载流程的context
    unsigned char       ota_dowload_time;
} http_instance_t;	

/*--------------------------------------------------------------------------
* 	                                        	Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                        	Global  Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */
extern http_instance_t g_http_instance;

/*--------------------------------------------------------------------------
* 	                                        	Function Prototypes
--------------------------------------------------------------------------*/
int http_client_download_request(void);
void http_client_instance_init(void);
int http_client_download_file(const char *url, unsigned int len);
http_instance_t *get_http_instance(void);
int http_get_port_from_url(char *url);
char *http_get_host_from_url(char *url);





#endif/*_HTTP_OTA_H*/

