#include "httpclient.h"
#include <http_log.h>
#include <http_error.h>
#include <http_event.h>
#include <platform_memory.h>

static void _http_client_variables_init(http_client_t *c)
{
    c->method = HTTP_REQUEST_METHOD_INVALID;
    c->interest_event = 0;
    c->process = 0;
    c->total = 0;
    c->data = NULL;
}

static int _http_client_internal_event_handle(void *e)
{
    http_event_t *event = e;
    http_interceptor_t *interceptor = event->context;
    http_client_t *c = (http_client_t*)interceptor->owner;
    int ret = HTTP_SUCCESS_ERROR;

    c->process = interceptor->data_process;
    c->total = http_response_get_length(&interceptor->response);
    
    if (0 == c->interest_event)
        RETURN_ERROR(HTTP_SUCCESS_ERROR);

    if (c->interest_event & event->type) {
        if (c->interest_event & http_event_type_on_request) {
            http_interceptor_set_keep_alive(interceptor);
            c->interest_event &= ~http_event_type_on_request;
        } else {
            ret = http_event_dispatch(c->event, event->type, c, event->data, event->len);
        }
    }
    return ret;
}

http_client_t *_http_client_create(void)
{
    http_client_t *c = NULL;
    int len = sizeof(http_client_t);
    
    c = platform_memory_alloc(len);
    
    memset(c, 0, len);

    if (NULL == c->connect_params) {
        c->connect_params = http_assign_connect_params();
    }

    if (NULL == c->event) {
        c->event = http_event_init();
    }

    if (NULL == c->interceptor) {
        len = sizeof(http_interceptor_t);
        c->interceptor = platform_memory_alloc(len);
        memset(c->interceptor, 0, len);
    }

    return c;
}

void _http_client_destroy(http_client_t *c)
{
    HTTP_ROBUSTNESS_CHECK(c, HTTP_VOID);

    if (c->connect_params) {
        http_release_connect_params(c->connect_params);
        c->connect_params = NULL;
    }

    if (c->event) {
        http_event_release(c->event);
        c->event = NULL;
    }

    if (c->interceptor) {
        platform_memory_free(c->interceptor);
        c->interceptor = NULL;
    }
}

int _http_client_handle(http_client_t *c, const char *url, http_request_method_t method, http_event_type_t et, http_event_cb_t cb, uint32_t flags)
{
    HTTP_ROBUSTNESS_CHECK(url, HTTP_SOCKET_FAILED_ERROR);
    HTTP_ROBUSTNESS_CHECK(c, HTTP_SOCKET_FAILED_ERROR);

    if (flags & http_keep_alive_flag)
        c->flag.flag_t.keep_alive = 1;

    http_client_set_interest_event(c, et);
    http_client_set_method(c, method);
    http_event_register(c->event, cb);
    http_url_parsing(c->connect_params, url);

    HTTP_LOG_D("\nhttp_interceptor_process start\n");
    int res = http_interceptor_process( c->interceptor, 
        c->connect_params, c->method, c->data, c,
        _http_client_internal_event_handle);
    HTTP_LOG_D("\nhttp_interceptor_process end%d\n", res);
    if (!c->flag.flag_t.keep_alive)
        http_client_release(c);

    return res;
}

http_client_t *http_client_init(const char *ca)
{
    http_client_t *c = _http_client_create();
    http_interceptor_set_ca(ca);

    return c;
}

void http_client_exit(http_client_t *c)
{
    _http_client_destroy(c);
    platform_memory_free(c);
}

void http_client_release(http_client_t *c)
{
    HTTP_ROBUSTNESS_CHECK(c, HTTP_VOID);

    _http_client_variables_init(c);
    http_interceptor_release(c->interceptor);
    http_release_connect_params_variables(c->connect_params);
}

void http_client_set_interest_event(http_client_t *c, http_event_type_t event)
{
    HTTP_ROBUSTNESS_CHECK((c && event), HTTP_VOID);

    c->interest_event = event;
}

void http_client_set_method(http_client_t *c, http_request_method_t method)
{
    HTTP_ROBUSTNESS_CHECK((c && method), HTTP_VOID);
    c->method = method;
}

void http_client_set_data(http_client_t *c, void *data)
{
    HTTP_ROBUSTNESS_CHECK(c, HTTP_VOID);
    c->data = data;
}

int http_client_method_request(http_client_t *c, http_request_method_t method, const char *url, http_event_cb_t cb)
{
    return _http_client_handle(c, url, method, http_event_type_on_body | http_event_type_on_headers, cb, 0);
}
