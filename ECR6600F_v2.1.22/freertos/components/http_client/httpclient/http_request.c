/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2020-05-05 17:20:36
 * @LastEditTime: 2020-06-03 19:32:43
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */

#include <http_request.h>
#include <http_log.h>
#include <http_error.h>
#include <http_list.h>
#include <platform_memory.h>

static const char *_http_request_version = HTTP_DEFAULT_VERSION;

static const char *_http_method_mapping[] = {"INVALID ", "GET ", "POST ", "HEAD ", "PUT ", "DELETE ", "OPTIONS ", "TRACE ", "CONNECT ", "PATCH " };

static const char *_http_request_headers_mapping[] = {
    "INVALID",
    "Cache-Control: ",
    "Connection: ",
    "Date: ",
    "Pragma: ",
    "Trailer: ",
    "Transfer-Encoding: ",
    "Upgrade: ",
    "Via: ",
    "Warning: ",
    "Accept: ",
    "Accept-Charset: ",
    "Accept-Encoding: ",
    "Accept-Language: ",
    "Authorization: ",
    "Expect: ",
    "From: ",
    "Host: ",
    "If-Match: ",
    "If-Modified-Since: ",
    "If-None-Match: ",
    "If-Range: ",
    "If-Unmodified-Since: ",
    "Max-Forwards: ",
    "Proxy-Authorization: ",
    "Range: ",
    "Referer: ",
    "TE: ",
    "User-Agent: ",
    "Allow: ",
    "Content-Length: ",
    "Content-Type: ",
    "X-Client-Mac: ",
    "X-Device-Id: ",
    "X-Device-Name: ",
    "X-Device-Model: ",
    "X-Device-Wifi-Ssid: ",
    "X-Device-Wifi-Bssid: ",
    "X-Device-Private-Ip-Addr: ",
    "X-Device-Location: ",
    "X-Platform-Name: ",
    "X-Platform-Version: ",
    "X-App-Version: "
};

typedef struct {
    http_list_t list;
    unsigned int key_index;
    char *value;
} http_user_header_t;

http_list_t g_user_header_list;

void http_request_user_msg_header_init(void)
{
    if (g_user_header_list.next == 0 && g_user_header_list.prev == 0) {
        http_list_init(&g_user_header_list);
    }
}

void http_request_user_msg_header_insert(http_request_t *req)
{
    http_list_t *list = NULL;
    http_user_header_t *msg_head = NULL;

    HTTP_LIST_FOR_EACH(list, &g_user_header_list) {
        msg_head = HTTP_LIST_ENTRY(list, http_user_header_t, list);
        http_request_add_header_form_index(req, msg_head->key_index, msg_head->value);
    }
}

void http_request_user_msg_header_release(unsigned int index)
{
    http_user_header_t *msg_head = NULL;
    if (index != HTTP_REQUEST_HEADER_INVALID) {
        http_list_t *list = NULL;

        HTTP_LIST_FOR_EACH(list, &g_user_header_list) {
            msg_head = HTTP_LIST_ENTRY(list, http_user_header_t, list);
            if (index == msg_head->key_index) {
                http_list_del(&msg_head->list);
                platform_memory_free(msg_head->value);
                platform_memory_free(msg_head);
                return;
            }
        }
    } else {
        msg_head = HTTP_LIST_FIRST_ENTRY_OR_NULL(&g_user_header_list, http_user_header_t, list);
        while (msg_head != NULL) {
            http_list_del(&msg_head->list);
            platform_memory_free(msg_head->value);
            platform_memory_free(msg_head);
            msg_head = HTTP_LIST_FIRST_ENTRY_OR_NULL(&g_user_header_list, http_user_header_t, list);
        }
    }
}

int http_request_user_msg_header_add(const char *key, unsigned int keylen, const char *value, unsigned int vlen)
{
    http_user_header_t *msg_head = NULL;
    unsigned int idx = HTTP_REQUEST_HEADER_INVALID;

    HTTP_ROBUSTNESS_CHECK(key, HTTP_NULL_VALUE_ERROR);
    http_request_user_msg_header_init();

    for (idx = HTTP_REQUEST_HEADER_INVALID; idx < HTTP_REQUEST_HEADER_MAX; idx++) {
        if (strncmp(_http_request_headers_mapping[idx], key, keylen) == 0) {
            break;
        }
    }

    if (idx >= HTTP_REQUEST_HEADER_MAX) {
        HTTP_LOG_E("header %s not support by default.\n", key);
        http_request_user_msg_header_release(HTTP_REQUEST_HEADER_INVALID);
        RETURN_ERROR(HTTP_FAILED_ERROR);
    }

    http_request_user_msg_header_release(idx);
    msg_head = platform_memory_alloc(sizeof(http_user_header_t));
    if (msg_head == NULL) {
        http_request_user_msg_header_release(HTTP_REQUEST_HEADER_INVALID);
        RETURN_ERROR(HTTP_FAILED_ERROR);
    }

    msg_head->key_index = idx;
    msg_head->value = platform_memory_alloc(vlen + 1);
    if (msg_head->value == NULL) {
        platform_memory_free(msg_head);
        http_request_user_msg_header_release(HTTP_REQUEST_HEADER_INVALID);
        RETURN_ERROR(HTTP_FAILED_ERROR);
    }
    memset(msg_head->value, 0, vlen + 1);
    if (vlen > 0) {
        memcpy(msg_head->value, value, vlen);
    }

    http_list_init(&msg_head->list);
    http_list_add(&msg_head->list, &g_user_header_list);
    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_init(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req , HTTP_NULL_VALUE_ERROR);

    req->req_msg.line = http_message_buffer_init(HTTP_MESSAGE_BUFFER_GROWTH);
    req->req_msg.header = http_message_buffer_init(HTTP_MESSAGE_BUFFER_GROWTH);
    req->req_msg.body = http_message_buffer_init(HTTP_MESSAGE_BUFFER_GROWTH);
    http_request_user_msg_header_init();

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_release(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req , HTTP_NULL_VALUE_ERROR);

    http_message_buffer_release(req->req_msg.line );
    http_message_buffer_release(req->req_msg.header);
    http_message_buffer_release(req->req_msg.body);

    req->req_msg.line = NULL;
    req->req_msg.header = NULL;
    req->req_msg.body = NULL;
    
    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}


int http_request_header_init(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req , HTTP_NULL_VALUE_ERROR);

    req->header_index[0] = 0;
    req->header_index[1] = 0;
    http_message_buffer_reinit(req->req_msg.header);
    
    http_request_user_msg_header_insert(req);
    http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_USER_AGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
    http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_ACCEPT, "*/*");

    if (req->flag.flag_t.keep_alive) {
        http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_CONNECTION, "keep-alive");
    } else {
        http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_CONNECTION, "Close");
    }

    http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_CONTENT_TYPE, "text/plain");

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}


int http_request_set_version(http_request_t *req, const char *str)
{
    HTTP_ROBUSTNESS_CHECK((req && str) , HTTP_NULL_VALUE_ERROR);
    if (http_utils_nmatch(str, "HTTP/", 5) == 0) {
        _http_request_version = str;
    } else {
        RETURN_ERROR(HTTP_NULL_VALUE_ERROR);
    }

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_set_keep_alive(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req , HTTP_NULL_VALUE_ERROR);
    req->flag.flag_t.keep_alive = 1;
    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_set_method(http_request_t *req,  http_request_method_t method)
{
    HTTP_ROBUSTNESS_CHECK((req && method), HTTP_NULL_VALUE_ERROR);

    req->method = method;

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_set_start_line(http_request_t *req, const char *path)
{
    HTTP_ROBUSTNESS_CHECK((req && path), HTTP_NULL_VALUE_ERROR);
    
    const char *m = _http_method_mapping[req->method];
    
    http_message_buffer_concat(req->req_msg.line, m, path, " ", _http_request_version, HTTP_CRLF, NULL);

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_set_start_line_with_query(http_request_t *req, const char *path, const char *query)
{
    HTTP_ROBUSTNESS_CHECK((req && path && query), HTTP_NULL_VALUE_ERROR);
    
    const char *m = _http_method_mapping[req->method];
    
    http_message_buffer_concat(req->req_msg.line, m, path, "?", query, " ", _http_request_version, HTTP_CRLF, NULL);

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

/* adds a header to the request with given key and value. */
void http_request_add_header(http_request_t *req, const char *key, const char *value)
{
    http_message_buffer_concat(req->req_msg.header, key, ": ", value, HTTP_CRLF, NULL);
}

/* adds a header to the request with given header index and value. */
void http_request_add_header_form_index(http_request_t *req, http_request_header_t index, const char *value)
{
    const char *key = _http_request_headers_mapping[index];
    uint32_t *hindex = &req->header_index[0];
    HTTP_ROBUSTNESS_CHECK((req && index && value), HTTP_VOID);

    if (index >= 32) {
        index -= 32;
        hindex = &req->header_index[1];
    }

    /* request header field already exists... */
    if (*hindex & (0x01 << index))
        return;
    *hindex |= (0x01 << index);

    http_message_buffer_concat(req->req_msg.header, key, value, HTTP_CRLF, NULL);
}

char *http_request_get_header(http_request_t *req, const char *key)
{
    HTTP_ROBUSTNESS_CHECK((req && key), NULL);
    
    int offset = strlen(key);
    char *addr = strstr(req->req_msg.header->data, key);

    if (addr)
        return (addr + offset);
    
    return NULL;
}

char *http_request_get_header_form_index(http_request_t *req, http_request_header_t index)
{
    const char *key = _http_request_headers_mapping[index];
    uint32_t *hindex = &req->header_index[0];
    HTTP_ROBUSTNESS_CHECK((req && index), NULL);

    if (index >= 32) {
        index -= 32;
        hindex = &req->header_index[1];
    }

    /* request header field already exists... */
    if (!(*hindex & (0x01 << index)))
        return NULL;

    return http_request_get_header(req, key);
}


int http_request_set_body(http_request_t *req, const char *buf, size_t size)
{
    HTTP_ROBUSTNESS_CHECK((req && buf && size), HTTP_NULL_VALUE_ERROR);

    http_message_buffer_concat(req->req_msg.body, buf, HTTP_CRLF, NULL);

    char *str = http_utils_itoa(req->req_msg.body->used, NULL, 10);
    http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_CONTENT_LENGTH, str);
    http_utils_release_string(str);

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

int http_request_set_body_form_pointer(http_request_t *req, const char *buf, size_t size)
{
    HTTP_ROBUSTNESS_CHECK((req && buf && size), HTTP_NULL_VALUE_ERROR);

    http_message_buffer_pointer(req->req_msg.body, buf, size);

    char *str = http_utils_itoa(req->req_msg.body->used, NULL, 10);
    http_request_add_header_form_index(req, HTTP_REQUEST_HEADER_CONTENT_LENGTH, str);
    http_utils_release_string(str);

    RETURN_ERROR(HTTP_SUCCESS_ERROR);
}

char *http_request_get_start_line_data(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, NULL);
    return req->req_msg.line->data;
}

char *http_request_get_header_data(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, NULL);
    return req->req_msg.header->data;
}

char *http_request_get_body_data(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, NULL);
    return req->req_msg.body->data;
}

size_t http_request_get_start_line_length(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, 0);
    return req->req_msg.line->length;
}

size_t http_request_get_header_length(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, 0);
    return req->req_msg.header->length;
}

size_t http_request_get_body_length(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, 0);
    return req->req_msg.body->length;
}

void http_request_print_start_line(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, HTTP_VOID);

    HTTP_LOG_I("%s:%d %s()...\n\n%s", __FILE__, __LINE__, __FUNCTION__, req->req_msg.line->data);
}

void http_request_print_header(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, HTTP_VOID);

    HTTP_LOG_I("%s:%d %s()...\n\n%s", __FILE__, __LINE__, __FUNCTION__, req->req_msg.header->data);
}

void http_request_print_body(http_request_t *req)
{
    HTTP_ROBUSTNESS_CHECK(req, HTTP_VOID);

    HTTP_LOG_I("%s:%d %s()...\n\n%s", __FILE__, __LINE__, __FUNCTION__, req->req_msg.body->data);
}

