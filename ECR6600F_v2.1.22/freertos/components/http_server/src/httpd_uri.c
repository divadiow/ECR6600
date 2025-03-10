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


#include <errno.h>
#include <http_parser.h>

#include <http_server_service.h>
#include "httpd_priv.h"

static bool httpd_uri_match_simple(const char *uri1, const char *uri2, size_t len2)
{
    return strlen(uri1) == len2 &&          // First match lengths
        (strncmp(uri1, uri2, len2) == 0);   // Then match actual URIs
}

bool httpd_uri_match_wildcard(const char *template, const char *uri, size_t len)
{
    const size_t tpl_len = strlen(template);
    size_t exact_match_chars = tpl_len;

    /* Check for trailing question mark and asterisk */
    const char last = (const char) (tpl_len > 0 ? template[tpl_len - 1] : 0);
    const char prevlast = (const char) (tpl_len > 1 ? template[tpl_len - 2] : 0);
    const bool asterisk = last == '*' || (prevlast == '*' && last == '?');
    const bool quest = last == '?' || (prevlast == '?' && last == '*');

    /* Minimum template string length must be:
     *      0 : if neither of '*' and '?' are present
     *      1 : if only '*' is present
     *      2 : if only '?' is present
     *      3 : if both are present
     *
     * The expression (asterisk + quest*2) serves as a
     * case wise generator of these length values
     */

    /* abort in cases such as "?" with no preceding character (invalid template) */
    if (exact_match_chars < asterisk + quest*2) {
        return false;
    }

    /* account for special characters and the optional character if "?" is used */
    exact_match_chars -= asterisk + quest*2;

    if (len < exact_match_chars) {
        return false;
    }

    if (!quest) {
        if (!asterisk && len != exact_match_chars) {
            /* no special characters and different length - strncmp would return false */
            return false;
        }
        /* asterisk allows arbitrary trailing characters, we ignore these using
         * exact_match_chars as the length limit */
        return (strncmp(template, uri, exact_match_chars) == 0);
    } else {
        /* question mark present */
        if (len > exact_match_chars && template[exact_match_chars] != uri[exact_match_chars]) {
            /* the optional character is present, but different */
            return false;
        }
        if (strncmp(template, uri, exact_match_chars) != 0) {
            /* the mandatory part differs */
            return false;
        }
        /* Now we know the URI is longer than the required part of template,
         * the mandatory part matches, and if the optional character is present, it is correct.
         * Match is OK if we have asterisk, i.e. any trailing characters are OK, or if
         * there are no characters beyond the optional character. */
        return asterisk || len <= exact_match_chars + 1;
    }
}

/* Find handler with matching URI and method, and set
 * appropriate error code if URI or method not found */
static httpd_uri_t* httpd_find_uri_handler(struct httpd_data *hd,
                                           const char *uri, size_t uri_len,
                                           httpd_method_t method,
                                           httpd_err_code_t *err)
{
    if (err) {
        *err = HTTPD_404_NOT_FOUND;
    }

    for (int i = 0; i < hd->config.max_uri_handlers; i++) {
        if (!hd->hd_calls[i]) {
            break;
        }
        os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] = %s"), i, hd->hd_calls[i]->uri);

        /* Check if custom URI matching function is set,
         * else use simple string compare */
        if (hd->config.uri_match_fn ?
            hd->config.uri_match_fn(hd->hd_calls[i]->uri, uri, uri_len) :
            httpd_uri_match_simple(hd->hd_calls[i]->uri, uri, uri_len)) {
            /* URIs match. Now check if method is supported */
            if (hd->hd_calls[i]->method == method) {
                /* Match found! */
                if (err) {
                    /* Unset any error that may
                     * have been set earlier */
                    *err = 0;
                }
                return hd->hd_calls[i];
            }
            /* URI found but method not allowed.
             * If URI is found later then this
             * error must be set to 0 */
            if (err) {
                *err = HTTPD_405_METHOD_NOT_ALLOWED;
            }
        }
    }
    return NULL;
}

int httpd_register_uri_handler(httpd_handle_t handle,
                                     const httpd_uri_t *uri_handler)
{
    if (handle == NULL || uri_handler == NULL) {
        return ERR_INVALID_ARG;
    }

    struct httpd_data *hd = (struct httpd_data *) handle;

    /* Make sure another handler with matching URI and method
     * is not already registered. This will also catch cases
     * when a registered URI wildcard pattern already accounts
     * for the new URI being registered */
    if (httpd_find_uri_handler(handle, uri_handler->uri,
                               strlen(uri_handler->uri),
                               uri_handler->method, NULL) != NULL) {
        os_printf(LM_APP, LL_WARN, LOG_FMT("handler %s with method %d already registered"),
                 uri_handler->uri, uri_handler->method);
        return ERR_HTTPD_HANDLER_EXISTS;
    }

    for (int i = 0; i < hd->config.max_uri_handlers; i++) {
        if (hd->hd_calls[i] == NULL) {
            hd->hd_calls[i] = malloc(sizeof(httpd_uri_t));
            if (hd->hd_calls[i] == NULL) {
                /* Failed to allocate memory */
                return ERR_HTTPD_ALLOC_MEM;
            }

            /* Copy URI string */
            hd->hd_calls[i]->uri = strdup(uri_handler->uri);
            if (hd->hd_calls[i]->uri == NULL) {
                /* Failed to allocate memory */
                free(hd->hd_calls[i]);
                return ERR_HTTPD_ALLOC_MEM;
            }

            /* Copy remaining members */
            hd->hd_calls[i]->method   = uri_handler->method;
            hd->hd_calls[i]->handler  = uri_handler->handler;
            hd->hd_calls[i]->user_ctx = uri_handler->user_ctx;
#ifdef CONFIG_HTTPD_WS_SUPPORT
            hd->hd_calls[i]->is_websocket = uri_handler->is_websocket;
            hd->hd_calls[i]->handle_ws_control_frames = uri_handler->handle_ws_control_frames;
            if (uri_handler->supported_subprotocol) {
                hd->hd_calls[i]->supported_subprotocol = strdup(uri_handler->supported_subprotocol);
            } else {
                hd->hd_calls[i]->supported_subprotocol = NULL;
            }
#endif
            os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] installed %s"), i, uri_handler->uri);
            return OS_SUCCESS;
        }
        os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] exists %s"), i, hd->hd_calls[i]->uri);
    }
    os_printf(LM_APP, LL_WARN, LOG_FMT("no slots left for registering handler"));
    return ERR_HTTPD_HANDLERS_FULL;
}

int httpd_unregister_uri_handler(httpd_handle_t handle,
                                       const char *uri, httpd_method_t method)
{
    if (handle == NULL || uri == NULL) {
        return ERR_INVALID_ARG;
    }

    struct httpd_data *hd = (struct httpd_data *) handle;
    for (int i = 0; i < hd->config.max_uri_handlers; i++) {
        if (!hd->hd_calls[i]) {
            break;
        }
        if ((hd->hd_calls[i]->method == method) &&       // First match methods
            (strcmp(hd->hd_calls[i]->uri, uri) == 0)) {  // Then match URI string
            os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] removing %s"), i, hd->hd_calls[i]->uri);

            free((char*)hd->hd_calls[i]->uri);
            free(hd->hd_calls[i]);
            hd->hd_calls[i] = NULL;

            /* Shift the remaining non null handlers in the array
             * forward by 1 so that order of insertion is maintained */
            for (i += 1; i < hd->config.max_uri_handlers; i++) {
                if (!hd->hd_calls[i]) {
                    break;
                }
                hd->hd_calls[i-1] = hd->hd_calls[i];
            }
            /* Nullify the following non null entry */
            hd->hd_calls[i-1] = NULL;
            return OS_SUCCESS;
        }
    }
    os_printf(LM_APP, LL_WARN, LOG_FMT("handler %s with method %d not found"), uri, method);
    return ERR_NOT_FOUND;
}

int httpd_unregister_uri(httpd_handle_t handle, const char *uri)
{
    if (handle == NULL || uri == NULL) {
        return ERR_INVALID_ARG;
    }

    struct httpd_data *hd = (struct httpd_data *) handle;
    bool found = false;

    int i = 0, j = 0; // For keeping count of removed entries
    for (; i < hd->config.max_uri_handlers; i++) {
        if (!hd->hd_calls[i]) {
            break;
        }
        if (strcmp(hd->hd_calls[i]->uri, uri) == 0) {   // Match URI strings
            os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] removing %s"), i, uri);

            free((char*)hd->hd_calls[i]->uri);
            free(hd->hd_calls[i]);
            hd->hd_calls[i] = NULL;
            found = true;

            j++; // Update count of removed entries
        } else {
            /* Shift the remaining non null handlers in the array
             * forward by j so that order of insertion is maintained */
            hd->hd_calls[i-j] = hd->hd_calls[i];
        }
    }
    /* Nullify the following non null entries */
    for (int k = (i - j); k < i; k++) {
        hd->hd_calls[k] = NULL;
    }

    if (!found) {
        os_printf(LM_APP, LL_WARN, LOG_FMT("no handler found for URI %s"), uri);
    }
    return (found ? OS_SUCCESS : ERR_NOT_FOUND);
}

void httpd_unregister_all_uri_handlers(struct httpd_data *hd)
{
    for (unsigned i = 0; i < hd->config.max_uri_handlers; i++) {
        if (!hd->hd_calls[i]) {
            break;
        }
        os_printf(LM_APP, LL_DBG, LOG_FMT("[%d] removing %s"), i, hd->hd_calls[i]->uri);

        free((char*)hd->hd_calls[i]->uri);
        free(hd->hd_calls[i]);
        hd->hd_calls[i] = NULL;
    }
}

int httpd_uri(struct httpd_data *hd)
{
    httpd_uri_t            *uri = NULL;
    httpd_req_t            *req = &hd->hd_req;
    struct http_parser_url *res = &hd->hd_req_aux.url_parse_res;

    /* For conveying URI not found/method not allowed */
    httpd_err_code_t err = 0;

    os_printf(LM_APP, LL_DBG, LOG_FMT("request for %s with type %d"), req->uri, req->method);

    /* URL parser result contains offset and length of path string */
    if (res->field_set & (1 << UF_PATH)) {
        uri = httpd_find_uri_handler(hd, req->uri + res->field_data[UF_PATH].off,
                                     res->field_data[UF_PATH].len, req->method, &err);
    }

    /* If URI with method not found, respond with error code */
    if (uri == NULL) {
        switch (err) {
            case HTTPD_404_NOT_FOUND:
                os_printf(LM_APP, LL_WARN, LOG_FMT("URI '%s' not found"), req->uri);
                return httpd_req_handle_err(req, HTTPD_404_NOT_FOUND);
            case HTTPD_405_METHOD_NOT_ALLOWED:
                os_printf(LM_APP, LL_WARN, LOG_FMT("Method '%d' not allowed for URI '%s'"),
                         req->method, req->uri);
                return httpd_req_handle_err(req, HTTPD_405_METHOD_NOT_ALLOWED);
            default:
                return OS_FAIL;
        }
    }

    /* Attach user context data (passed during URI registration) into request */
    req->user_ctx = uri->user_ctx;

    /* Final step for a WebSocket handshake verification */
#ifdef CONFIG_HTTPD_WS_SUPPORT
    struct httpd_req_aux   *aux = req->aux;
    if (uri->is_websocket && aux->ws_handshake_detect && uri->method == HTTP_GET) {
        os_printf(LM_APP, LL_DBG, LOG_FMT("Responding WS handshake to sock %d"), aux->sd->fd);
        int ret = httpd_ws_respond_server_handshake(&hd->hd_req, uri->supported_subprotocol);
        if (ret != OS_SUCCESS) {
            return ret;
        }

        aux->sd->ws_handshake_done = true;
        aux->sd->ws_handler = uri->handler;
        aux->sd->ws_control_frames = uri->handle_ws_control_frames;

        /* Return immediately after handshake, no need to call handler here */
        return OS_SUCCESS;
    }
#endif

    /* Invoke handler */
    if (uri->handler(req) != OS_SUCCESS) {
        /* Handler returns error, this socket should be closed */
        os_printf(LM_APP, LL_WARN, LOG_FMT("uri handler execution failed"));
        return OS_FAIL;
    }
    return OS_SUCCESS;
}
