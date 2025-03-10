/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

//add by wangfei
#include "easyflash.h"
#include "system_wifi.h"
#include "http_os.h"
#include "cli.h"

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

typedef const char*  event_base_t; /**< unique pointer to a subsystem that exposes events */
typedef void*        event_loop_handle_t; /**< a number that identifies an event with respect to a base */
typedef void         (*event_handler_t)(void* event_handler_arg,
                                        event_base_t event_base,
                                        int32_t event_id,
                                        void* event_data); /**< function called when an event is posted to the queue */
typedef void*        event_handler_instance_t; /**< context identifying an instance of a registered event handler */
//add by wangfei
#include <http_server_service.h>
#include "setting_ssid.h"
#include "cJSON.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static int setting_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)setting_ssid_html_gz, setting_ssid_html_gz_len); 
}

httpd_uri_t setting_get = {
    .uri       = "/setting",
    .method    = HTTP_GET,
    .handler   = setting_handler,
    .user_ctx  = NULL
};


/* An HTTP POST handler */
static int setting_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return OS_FAIL;
        }

        /* Send back the same data */
        char* notice = "Setting Succeed!";
        httpd_resp_send_chunk(req, notice, strlen(notice)+1);
        remaining -= ret;

        /* my json code begin */
        const char * json = buf;
        cJSON * root_base = cJSON_Parse(json);//change to json object
        if(root_base){ 
   
            cJSON * cParse1 = cJSON_GetObjectItem(root_base,"ssid");
            cJSON * cParse2 = cJSON_GetObjectItem(root_base,"pwd");	 

            customer_set_env_blob("UserSSID", cParse1 -> valuestring, strlen(cParse1 -> valuestring)+1);
            customer_set_env_blob("UserPWD", cParse2 -> valuestring, strlen(cParse2 -> valuestring)+1);  
            system_printf("\nssid = %s\npassword = %s\n",cParse1 -> valuestring,cParse2 -> valuestring);
 
            cJSON_Delete(root_base);
        }
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return OS_SUCCESS;
}

static const httpd_uri_t setting_post = {
    .uri       = "/setting",
    .method    = HTTP_POST,
    .handler   = setting_post_handler,
    .user_ctx  = NULL
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    os_printf(LM_APP, LL_DBG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == OS_SUCCESS) {
        // Set URI handlers
        os_printf(LM_APP, LL_DBG, "Registering URI handlers");
        httpd_register_uri_handler(server, &setting_get);
        httpd_register_uri_handler(server, &setting_post);
        return server; 
    }

    os_printf(LM_APP, LL_DBG, "Error starting server!");
    return NULL; 
}

httpd_handle_t webconfig_main(void)
{
    httpd_handle_t server = start_webserver();
    return server;
}
