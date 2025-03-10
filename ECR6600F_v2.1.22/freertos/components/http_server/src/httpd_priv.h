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


#ifndef _HTTPD_PRIV_H_
#define _HTTPD_PRIV_H_

#include <stdbool.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <http_server_service.h>
#include "http_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size of request data block/chunk (not to be confused with chunked encoded data)
 * that is received and parsed in one turn of the parsing process. This should not
 * exceed the scratch buffer size and should at least be 8 bytes */
#define PARSER_BLOCK_SIZE  128

/* Calculate the maximum size needed for the scratch buffer */
#define HTTPD_SCRATCH_BUF  MAX(HTTPD_MAX_REQ_HDR_LEN, HTTPD_MAX_URI_LEN)

/* Formats a log string to prepend context function name */
#define LOG_FMT(x)      "%s: " x, __func__

/**
 * @brief Thread related data for internal use
 */
struct thread_data {
    othread_t handle;   /*!< Handle to thread/task */
    enum {
        THREAD_IDLE = 0,
        THREAD_RUNNING,
        THREAD_STOPPING,
        THREAD_STOPPED,
    } status;           /*!< State of the thread */
};

/**
 * @brief A database of all the open sockets in the system.
 */
struct sock_db {
    int fd;                                 /*!< The file descriptor for this socket */
    void *ctx;                              /*!< A custom context for this socket */
    bool ignore_sess_ctx_changes;           /*!< Flag indicating if session context changes should be ignored */
    void *transport_ctx;                    /*!< A custom 'transport' context for this socket, to be used by send/recv/pending */
    httpd_handle_t handle;                  /*!< Server handle */
    httpd_free_ctx_fn_t free_ctx;      /*!< Function for freeing the context */
    httpd_free_ctx_fn_t free_transport_ctx; /*!< Function for freeing the 'transport' context */
    httpd_send_func_t send_fn;              /*!< Send function for this socket */
    httpd_recv_func_t recv_fn;              /*!< Receive function for this socket */
    httpd_pending_func_t pending_fn;        /*!< Pending function for this socket */
    uint64_t lru_counter;                   /*!< LRU Counter indicating when the socket was last used */
    bool lru_socket;                        /*!< Flag indicating LRU socket */
    char pending_data[PARSER_BLOCK_SIZE];   /*!< Buffer for pending data to be received */
    size_t pending_len;                     /*!< Length of pending data to be received */
#ifdef CONFIG_HTTPD_WS_SUPPORT
    bool ws_handshake_done;                 /*!< True if it has done WebSocket handshake (if this socket is a valid WS) */
    bool ws_close;                          /*!< Set to true to close the socket later (when WS Close frame received) */
    int (*ws_handler)(httpd_req_t *r);   /*!< WebSocket handler, leave to null if it's not WebSocket */
    bool ws_control_frames;                         /*!< WebSocket flag indicating that control frames should be passed to user handlers */
#endif
};

/**
 * @brief   Auxiliary data structure for use during reception and processing
 *          of requests and temporarily keeping responses
 */
struct httpd_req_aux {
    struct sock_db *sd;                             /*!< Pointer to socket database */
    char            scratch[HTTPD_SCRATCH_BUF + 1]; /*!< Temporary buffer for our operations (1 byte extra for null termination) */
    size_t          remaining_len;                  /*!< Amount of data remaining to be fetched */
    char           *status;                         /*!< HTTP response's status code */
    char           *content_type;                   /*!< HTTP response's content type */
    bool            first_chunk_sent;               /*!< Used to indicate if first chunk sent */
    unsigned        req_hdrs_count;                 /*!< Count of total headers in request packet */
    unsigned        resp_hdrs_count;                /*!< Count of additional headers in response packet */
    struct resp_hdr {
        const char *field;
        const char *value;
    } *resp_hdrs;                                   /*!< Additional headers in response packet */
    struct http_parser_url url_parse_res;           /*!< URL parsing result, used for retrieving URL elements */
#ifdef CONFIG_HTTPD_WS_SUPPORT
    bool ws_handshake_detect;                       /*!< WebSocket handshake detection flag */
    httpd_ws_type_t ws_type;                        /*!< WebSocket frame type */
    bool ws_final;                                  /*!< WebSocket FIN bit (final frame or not) */
#endif
};

/**
 * @brief   Server data for each instance. This is exposed publicly as
 *          httpd_handle_t but internal structure/members are kept private.
 */
struct httpd_data {
    httpd_config_t config;                  /*!< HTTPD server configuration */
    int listen_fd;                          /*!< Server listener FD */
    int ctrl_fd;                            /*!< Ctrl message receiver FD */
    int msg_fd;                             /*!< Ctrl message sender FD */
    struct thread_data hd_td;               /*!< Information for the HTTPD thread */
    struct sock_db *hd_sd;                  /*!< The socket database */
    httpd_uri_t **hd_calls;                 /*!< Registered URI handlers */
    struct httpd_req hd_req;                /*!< The current HTTPD request */
    struct httpd_req_aux hd_req_aux;        /*!< Additional data about the HTTPD request kept unexposed */

    /* Array of registered error handler functions */
    httpd_err_handler_func_t *err_handler_fns;
};

struct sock_db *httpd_sess_get(struct httpd_data *hd, int sockfd);
void httpd_sess_delete_invalid(struct httpd_data *hd);
void httpd_sess_init(struct httpd_data *hd);
int httpd_sess_new(struct httpd_data *hd, int newfd);
int httpd_sess_process(struct httpd_data *hd, int clifd);
int httpd_sess_delete(struct httpd_data *hd, int clifd);
void httpd_sess_free_ctx(void *ctx, httpd_free_ctx_fn_t free_fn);
void httpd_sess_set_descriptors(struct httpd_data *hd, fd_set *fdset, int *maxfd);
int httpd_sess_iterate(struct httpd_data *hd, int fd);
bool httpd_is_sess_available(struct httpd_data *hd);
bool httpd_sess_pending(struct httpd_data *hd, int fd);
int httpd_sess_close_lru(struct httpd_data *hd);
int httpd_uri(struct httpd_data *hd);
void httpd_unregister_all_uri_handlers(struct httpd_data *hd);
bool httpd_validate_req_ptr(httpd_req_t *r);

#ifdef CONFIG_HTTPD_VALIDATE_REQ
#define httpd_valid_req(r)  httpd_validate_req_ptr(r)
#else
#define httpd_valid_req(r)  true
#endif

int httpd_req_new(struct httpd_data *hd, struct sock_db *sd);
int httpd_req_delete(struct httpd_data *hd);
int httpd_req_handle_err(httpd_req_t *req, httpd_err_code_t error);
int httpd_send(httpd_req_t *req, const char *buf, size_t buf_len);
int httpd_recv_with_opt(httpd_req_t *r, char *buf, size_t buf_len, bool halt_after_pending);
size_t httpd_unrecv(struct httpd_req *r, const char *buf, size_t buf_len);
int httpd_default_send(httpd_handle_t hd, int sockfd, const char *buf, size_t buf_len, int flags);
int httpd_default_recv(httpd_handle_t hd, int sockfd, char *buf, size_t buf_len, int flags);
int httpd_ws_respond_server_handshake(httpd_req_t *req, const char *supported_subprotocol);
int httpd_ws_get_frame_type(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* ! _HTTPD_PRIV_H_ */
