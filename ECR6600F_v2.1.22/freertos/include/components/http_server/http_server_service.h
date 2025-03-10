// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __HTTP_SERVER_SERVICE_H__
#define __HTTP_SERVER_SERVICE_H__

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

//add by wangfei start
#include "lwip/netdb.h"
#include <fcntl.h>
#include "oshal.h"//wangfei
#include "rtos_debug.h"//wangfei
#include "cli.h"//wangfei
//typedef int bool;


//add by wangfei end
/*
note: esp_https_server.h includes a customized copy of this
initializer that should be kept in sync
*/
#define HTTPD_DEFAULT_CONFIG() {                        \
        .task_priority      = tskIDLE_PRIORITY+5,       \
        .stack_size         = 4096,                     \
        .core_id            = 0,           \
        .server_port        = 80,                       \
        .ctrl_port          = 32768,                    \
        .max_open_sockets   = 7,                        \
        .max_uri_handlers   = 8,                        \
        .max_resp_headers   = 8,                        \
        .backlog_conn       = 5,                        \
        .lru_purge_enable   = false,                    \
        .recv_wait_timeout  = 5,                        \
        .send_wait_timeout  = 5,                        \
        .global_user_ctx = NULL,                        \
        .global_user_ctx_free_fn = NULL,                \
        .global_transport_ctx = NULL,                   \
        .global_transport_ctx_free_fn = NULL,           \
        .open_fn = NULL,                                \
        .close_fn = NULL,                               \
        .uri_match_fn = NULL                            \
}

/* Symbol to be used as length parameter in httpd_resp_send APIs
 * for setting buffer length to string length */
#define HTTPD_RESP_USE_STRLEN -1

/* ************** Group: Initialization ************** */
/** @name Initialization
 * APIs related to the Initialization of the web server
 * @{
 */

/**
 * @brief   HTTP Server Instance Handle
 *
 * Every instance of the server will have a unique handle.
 */
typedef void* httpd_handle_t;

/**
 * @brief   HTTP Method Type wrapper over "enum http_method"
 *          available in "http_parser" library
 */
typedef enum http_method httpd_method_t;
typedef void (*httpd_free_ctx_fn_t)(void *ctx);
typedef int (*httpd_open_func_t)(httpd_handle_t hd, int sockfd);
typedef void (*httpd_close_func_t)(httpd_handle_t hd, int sockfd);
typedef bool (*httpd_uri_match_func_t)(const char *reference_uri,
                                       const char *uri_to_match,
                                       size_t match_upto);

typedef struct httpd_config {
    unsigned    task_priority;      /*!< Priority of FreeRTOS task which runs the server */
    size_t      stack_size;         /*!< The maximum stack size allowed for the server task */
    BaseType_t  core_id;            /*!< The core the HTTP server task will run on */

    /**
     * TCP Port number for receiving and transmitting HTTP traffic
     */
    uint16_t    server_port;

    /**
     * UDP Port number for asynchronously exchanging control signals
     * between various components of the server
     */
    uint16_t    ctrl_port;

    uint16_t    max_open_sockets;   /*!< Max number of sockets/clients connected at any time*/
    uint16_t    max_uri_handlers;   /*!< Maximum allowed uri handlers */
    uint16_t    max_resp_headers;   /*!< Maximum allowed additional headers in HTTP response */
    uint16_t    backlog_conn;       /*!< Number of backlog connections */
    bool        lru_purge_enable;   /*!< Purge "Least Recently Used" connection */
    uint16_t    recv_wait_timeout;  /*!< Timeout for recv function (in seconds)*/
    uint16_t    send_wait_timeout;  /*!< Timeout for send function (in seconds)*/

    /**
     * Global user context.
     *
     * This field can be used to store arbitrary user data within the server context.
     * The value can be retrieved using the server handle, available e.g. in the httpd_req_t struct.
     *
     * When shutting down, the server frees up the user context by
     * calling free() on the global_user_ctx field. If you wish to use a custom
     * function for freeing the global user context, please specify that here.
     */
    void * global_user_ctx;

    /**
     * Free function for global user context
     */
    httpd_free_ctx_fn_t global_user_ctx_free_fn;

    /**
     * Global transport context.
     *
     * Similar to global_user_ctx, but used for session encoding or encryption (e.g. to hold the SSL context).
     * It will be freed using free(), unless global_transport_ctx_free_fn is specified.
     */
    void * global_transport_ctx;

    /**
     * Free function for global transport context
     */
    httpd_free_ctx_fn_t global_transport_ctx_free_fn;

    /**
     * Custom session opening callback.
     *
     * Called on a new session socket just after accept(), but before reading any data.
     *
     * This is an opportunity to set up e.g. SSL encryption using global_transport_ctx
     * and the send/recv/pending session overrides.
     *
     * If a context needs to be maintained between these functions, store it in the session using
     * httpd_sess_set_transport_ctx() and retrieve it later with httpd_sess_get_transport_ctx()
     *
     * Returning a value other than ESP_OK will immediately close the new socket.
     */
    httpd_open_func_t open_fn;

    /**
     * Custom session closing callback.
     *
     * Called when a session is deleted, before freeing user and transport contexts and before
     * closing the socket. This is a place for custom de-init code common to all sockets.
     *
     * Set the user or transport context to NULL if it was freed here, so the server does not
     * try to free it again.
     *
     * This function is run for all terminated sessions, including sessions where the socket
     * was closed by the network stack - that is, the file descriptor may not be valid anymore.
     */
    httpd_close_func_t close_fn;

    /**
     * URI matcher function.
     *
     * Called when searching for a matching URI:
     *     1) whose request handler is to be executed right
     *        after an HTTP request is successfully parsed
     *     2) in order to prevent duplication while registering
     *        a new URI handler using `httpd_register_uri_handler()`
     *
     * Available options are:
     *     1) NULL : Internally do basic matching using `strncmp()`
     *     2) `httpd_uri_match_wildcard()` : URI wildcard matcher
     *
     * Users can implement their own matching functions (See description
     * of the `httpd_uri_match_func_t` function prototype)
     */
    httpd_uri_match_func_t uri_match_fn;
} httpd_config_t;

int  httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
int httpd_stop(httpd_handle_t handle);
#define HTTPD_MAX_REQ_HDR_LEN CONFIG_HTTPD_MAX_REQ_HDR_LEN

/* Max supported HTTP request URI length */
#define HTTPD_MAX_URI_LEN CONFIG_HTTPD_MAX_URI_LEN
typedef struct httpd_req {
    httpd_handle_t  handle;                     /*!< Handle to server instance */
    int             method;                     /*!< The type of HTTP request, -1 if unsupported method */
    const char      uri[HTTPD_MAX_URI_LEN + 1]; /*!< The URI of this request (1 byte extra for null termination) */
    size_t          content_len;                /*!< Length of the request body */
    void           *aux;                        /*!< Internally used members */

    /**
     * User context pointer passed during URI registration.
     */
    void *user_ctx;

    /**
     * Session Context Pointer
     *
     * A session context. Contexts are maintained across 'sessions' for a
     * given open TCP connection. One session could have multiple request
     * responses. The web server will ensure that the context persists
     * across all these request and responses.
     *
     * By default, this is NULL. URI Handlers can set this to any meaningful
     * value.
     *
     * If the underlying socket gets closed, and this pointer is non-NULL,
     * the web server will free up the context by calling free(), unless
     * free_ctx function is set.
     */
    void *sess_ctx;

    /**
     * Pointer to free context hook
     *
     * Function to free session context
     *
     * If the web server's socket closes, it frees up the session context by
     * calling free() on the sess_ctx member. If you wish to use a custom
     * function for freeing the session context, please specify that here.
     */
    httpd_free_ctx_fn_t free_ctx;

    /**
     * Flag indicating if Session Context changes should be ignored
     *
     * By default, if you change the sess_ctx in some URI handler, the http server
     * will internally free the earlier context (if non NULL), after the URI handler
     * returns. If you want to manage the allocation/reallocation/freeing of
     * sess_ctx yourself, set this flag to true, so that the server will not
     * perform any checks on it. The context will be cleared by the server
     * (by calling free_ctx or free()) only if the socket gets closed.
     */
    bool ignore_sess_ctx_changes;
} httpd_req_t;

/**
 * @brief Structure for URI handler
 */
typedef struct httpd_uri {
    const char       *uri;    /*!< The URI to handle */
    httpd_method_t    method; /*!< Method supported by the URI */
    int (*handler)(httpd_req_t *r);

    /**
     * Pointer to user context data which will be available to handler
     */
    void *user_ctx;

#ifdef CONFIG_HTTPD_WS_SUPPORT
    /**
     * Flag for indicating a WebSocket endpoint.
     * If this flag is true, then method must be HTTP_GET. Otherwise the handshake will not be handled.
     */
    bool is_websocket;

    /**
     * Flag indicating that control frames (PING, PONG, CLOSE) are also passed to the handler
     * This is used if a custom processing of the control frames is needed
     */
    bool handle_ws_control_frames;

    /**
     * Pointer to subprotocol supported by URI
     */
    const char *supported_subprotocol;
#endif
} httpd_uri_t;

int httpd_register_uri_handler(httpd_handle_t handle,
                                     const httpd_uri_t *uri_handler);
int httpd_unregister_uri_handler(httpd_handle_t handle,
                                       const char *uri, httpd_method_t method);
int httpd_unregister_uri(httpd_handle_t handle, const char* uri);
typedef enum {
    /* For any unexpected errors during parsing, like unexpected
     * state transitions, or unhandled errors.
     */
    HTTPD_500_INTERNAL_SERVER_ERROR = 0,

    /* For methods not supported by http_parser. Presently
     * http_parser halts parsing when such methods are
     * encountered and so the server responds with 400 Bad
     * Request error instead.
     */
    HTTPD_501_METHOD_NOT_IMPLEMENTED,

    /* When HTTP version is not 1.1 */
    HTTPD_505_VERSION_NOT_SUPPORTED,

    /* Returned when http_parser halts parsing due to incorrect
     * syntax of request, unsupported method in request URI or
     * due to chunked encoding / upgrade field present in headers
     */
    HTTPD_400_BAD_REQUEST,

    /* This response means the client must authenticate itself
     * to get the requested response.
     */
    HTTPD_401_UNAUTHORIZED,

    /* The client does not have access rights to the content,
     * so the server is refusing to give the requested resource.
     * Unlike 401, the client's identity is known to the server.
     */
    HTTPD_403_FORBIDDEN,

    /* When requested URI is not found */
    HTTPD_404_NOT_FOUND,

    /* When URI found, but method has no handler registered */
    HTTPD_405_METHOD_NOT_ALLOWED,

    /* Intended for recv timeout. Presently it's being sent
     * for other recv errors as well. Client should expect the
     * server to immediately close the connection after
     * responding with this.
     */
    HTTPD_408_REQ_TIMEOUT,

    /* Intended for responding to chunked encoding, which is
     * not supported currently. Though unhandled http_parser
     * callback for chunked request returns "400 Bad Request"
     */
    HTTPD_411_LENGTH_REQUIRED,

    /* URI length greater than CONFIG_HTTPD_MAX_URI_LEN */
    HTTPD_414_URI_TOO_LONG,

    /* Headers section larger than CONFIG_HTTPD_MAX_REQ_HDR_LEN */
    HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE,

    /* Used internally for retrieving the total count of errors */
    HTTPD_ERR_CODE_MAX
} httpd_err_code_t;

typedef int (*httpd_err_handler_func_t)(httpd_req_t *req,
                                              httpd_err_code_t error);

int httpd_register_err_handler(httpd_handle_t handle,
                                     httpd_err_code_t error,
                                     httpd_err_handler_func_t handler_fn);

#define HTTPD_SOCK_ERR_FAIL      -1
#define HTTPD_SOCK_ERR_INVALID   -2
#define HTTPD_SOCK_ERR_TIMEOUT   -3

typedef int (*httpd_send_func_t)(httpd_handle_t hd, int sockfd, const char *buf, size_t buf_len, int flags);
typedef int (*httpd_recv_func_t)(httpd_handle_t hd, int sockfd, char *buf, size_t buf_len, int flags);
typedef int (*httpd_pending_func_t)(httpd_handle_t hd, int sockfd);
int httpd_sess_set_recv_override(httpd_handle_t hd, int sockfd, httpd_recv_func_t recv_func);
int httpd_sess_set_send_override(httpd_handle_t hd, int sockfd, httpd_send_func_t send_func);
int httpd_sess_set_pending_override(httpd_handle_t hd, int sockfd, httpd_pending_func_t pending_func);
int httpd_req_to_sockfd(httpd_req_t *r);
int httpd_req_recv(httpd_req_t *r, char *buf, size_t buf_len);
size_t httpd_req_get_hdr_value_len(httpd_req_t *r, const char *field);
int httpd_req_get_hdr_value_str(httpd_req_t *r, const char *field, char *val, size_t val_size);
size_t httpd_req_get_url_query_len(httpd_req_t *r);
int httpd_req_get_url_query_str(httpd_req_t *r, char *buf, size_t buf_len);
int httpd_query_key_value(const char *qry, const char *key, char *val, size_t val_size);
bool httpd_uri_match_wildcard(const char *uri_template, const char *uri_to_match, size_t match_upto);
int httpd_resp_send(httpd_req_t *r, const char *buf, ssize_t buf_len);
int httpd_resp_send_chunk(httpd_req_t *r, const char *buf, ssize_t buf_len);
static inline int httpd_resp_sendstr(httpd_req_t *r, const char *str) {
    return httpd_resp_send(r, str, (str == NULL) ? 0 : HTTPD_RESP_USE_STRLEN);
}

static inline int httpd_resp_sendstr_chunk(httpd_req_t *r, const char *str) {
    return httpd_resp_send_chunk(r, str, (str == NULL) ? 0 : HTTPD_RESP_USE_STRLEN);
}

/* Some commonly used status codes */
#define HTTPD_200      "200 OK"                     /*!< HTTP Response 200 */
#define HTTPD_204      "204 No Content"             /*!< HTTP Response 204 */
#define HTTPD_207      "207 Multi-Status"           /*!< HTTP Response 207 */
#define HTTPD_400      "400 Bad Request"            /*!< HTTP Response 400 */
#define HTTPD_404      "404 Not Found"              /*!< HTTP Response 404 */
#define HTTPD_408      "408 Request Timeout"        /*!< HTTP Response 408 */
#define HTTPD_500      "500 Internal Server Error"  /*!< HTTP Response 500 */
int httpd_resp_set_status(httpd_req_t *r, const char *status);

/* Some commonly used content types */
#define HTTPD_TYPE_JSON   "application/json"            /*!< HTTP Content type JSON */
#define HTTPD_TYPE_TEXT   "text/html"                   /*!< HTTP Content type text/HTML */
#define HTTPD_TYPE_OCTET  "application/octet-stream"    /*!< HTTP Content type octext-stream */
int httpd_resp_set_type(httpd_req_t *r, const char *type);
int httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value);
int httpd_resp_send_err(httpd_req_t *req, httpd_err_code_t error, const char *msg);
static inline int httpd_resp_send_404(httpd_req_t *r) {
    return httpd_resp_send_err(r, HTTPD_404_NOT_FOUND, NULL);
}

static inline int httpd_resp_send_408(httpd_req_t *r) {
    return httpd_resp_send_err(r, HTTPD_408_REQ_TIMEOUT, NULL);
}

static inline int httpd_resp_send_500(httpd_req_t *r) {
    return httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
}

int httpd_send(httpd_req_t *r, const char *buf, size_t buf_len);
int httpd_socket_send(httpd_handle_t hd, int sockfd, const char *buf, size_t buf_len, int flags);
int httpd_socket_recv(httpd_handle_t hd, int sockfd, char *buf, size_t buf_len, int flags);
void *httpd_sess_get_ctx(httpd_handle_t handle, int sockfd);

/**
 * @brief   Set session context by socket descriptor
 *
 * @param[in] handle    Handle to server returned by httpd_start
 * @param[in] sockfd    The socket descriptor for which the context should be extracted.
 * @param[in] ctx       Context object to assign to the session
 * @param[in] free_fn   Function that should be called to free the context
 */
void httpd_sess_set_ctx(httpd_handle_t handle, int sockfd, void *ctx, httpd_free_ctx_fn_t free_fn);

/**
 * @brief   Get session 'transport' context by socket descriptor
 * @see     httpd_sess_get_ctx()
 *
 * This context is used by the send/receive functions, for example to manage SSL context.
 *
 * @param[in] handle    Handle to server returned by httpd_start
 * @param[in] sockfd    The socket descriptor for which the context should be extracted.
 * @return
 *  - void* : Pointer to the transport context associated with this session
 *  - NULL  : Empty context / Invalid handle / Invalid socket fd
 */
void *httpd_sess_get_transport_ctx(httpd_handle_t handle, int sockfd);

/**
 * @brief   Set session 'transport' context by socket descriptor
 * @see     httpd_sess_set_ctx()
 *
 * @param[in] handle    Handle to server returned by httpd_start
 * @param[in] sockfd    The socket descriptor for which the context should be extracted.
 * @param[in] ctx       Transport context object to assign to the session
 * @param[in] free_fn   Function that should be called to free the transport context
 */
void httpd_sess_set_transport_ctx(httpd_handle_t handle, int sockfd, void *ctx, httpd_free_ctx_fn_t free_fn);

/**
 * @brief   Get HTTPD global user context (it was set in the server config struct)
 *
 * @param[in] handle    Handle to server returned by httpd_start
 * @return global user context
 */
void *httpd_get_global_user_ctx(httpd_handle_t handle);

/**
 * @brief   Get HTTPD global transport context (it was set in the server config struct)
 *
 * @param[in] handle    Handle to server returned by httpd_start
 * @return global transport context
 */
void *httpd_get_global_transport_ctx(httpd_handle_t handle);
int httpd_sess_trigger_close(httpd_handle_t handle, int sockfd);
int httpd_sess_update_lru_counter(httpd_handle_t handle, int sockfd);
int httpd_get_client_list(httpd_handle_t handle, size_t *fds, int *client_fds);
typedef void (*httpd_work_fn_t)(void *arg);
int httpd_queue_work(httpd_handle_t handle, httpd_work_fn_t work, void *arg);
#ifdef CONFIG_HTTPD_WS_SUPPORT
/**
 * @brief Enum for WebSocket packet types (Opcode in the header)
 * @note Please refer to RFC6455 Section 5.4 for more details
 */
typedef enum {
    HTTPD_WS_TYPE_CONTINUE   = 0x0,
    HTTPD_WS_TYPE_TEXT       = 0x1,
    HTTPD_WS_TYPE_BINARY     = 0x2,
    HTTPD_WS_TYPE_CLOSE      = 0x8,
    HTTPD_WS_TYPE_PING       = 0x9,
    HTTPD_WS_TYPE_PONG       = 0xA
} httpd_ws_type_t;

/**
 * @brief Enum for client info description
 */
typedef enum {
    HTTPD_WS_CLIENT_INVALID        = 0x0,
    HTTPD_WS_CLIENT_HTTP           = 0x1,
    HTTPD_WS_CLIENT_WEBSOCKET      = 0x2,
} httpd_ws_client_info_t;

/**
 * @brief WebSocket frame format
 */
typedef struct httpd_ws_frame {
    bool final;                 /*!< Final frame:
                                     For received frames this field indicates whether the `FIN` flag was set.
                                     For frames to be transmitted, this field is only used if the `fragmented`
                                         option is set as well. If `fragmented` is false, the `FIN` flag is set
                                         by default, marking the ws_frame as a complete/unfragmented message
                                         (esp_http_server doesn't automatically fragment messages) */
    bool fragmented;            /*!< Indication that the frame allocated for transmission is a message fragment,
                                     so the `FIN` flag is set manually according to the `final` option.
                                     This flag is never set for received messages */
    httpd_ws_type_t type;       /*!< WebSocket frame type */
    uint8_t *payload;           /*!< Pre-allocated data buffer */
    size_t len;                 /*!< Length of the WebSocket data */
} httpd_ws_frame_t;
int httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *pkt, size_t max_len);
int httpd_ws_send_frame(httpd_req_t *req, httpd_ws_frame_t *pkt);
int httpd_ws_send_frame_async(httpd_handle_t hd, int fd, httpd_ws_frame_t *frame);
httpd_ws_client_info_t httpd_ws_get_fd_info(httpd_handle_t hd, int fd);

#endif /* CONFIG_HTTPD_WS_SUPPORT */
/** End of WebSocket related stuff
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
