
#include <stdlib.h>
#include <string.h>
//#include "defs.h"

#include "system_queue.h"
#include "mqtt_outbox.h"




typedef struct outbox_item {
    char *buffer;
    int len;
    int msg_id;
    int msg_type;
    int tick;
    int retry_count;
    BOOL pending;
    STAILQ_ENTRY(outbox_item) next;
} outbox_item_t;

STAILQ_HEAD(outbox_list_t, outbox_item);


outbox_handle_t outbox_init()
{
    outbox_handle_t outbox = mqtt_calloc(1, sizeof(struct outbox_list_t));
    MEM_CHECK(outbox, return NULL);
    STAILQ_INIT(outbox);
    return outbox;
}

outbox_item_handle_t outbox_enqueue(outbox_handle_t outbox, uint8_t *data, int len, int msg_id, int msg_type, int tick)
{
    outbox_item_handle_t item = mqtt_calloc(1, sizeof(outbox_item_t));
    MEM_CHECK(item, return NULL);
    item->msg_id = msg_id;
    item->msg_type = msg_type;
    item->tick = tick;
    item->len = len;
    item->buffer = mqtt_malloc(len);
    MEM_CHECK(item->buffer, {
        mqtt_free(item);
        return NULL;
    });
    memcpy(item->buffer, data, len);
    STAILQ_INSERT_TAIL(outbox, item, next);
    os_printf(LM_APP, LL_INFO, "MQTT:ENQUEUE msgid=%d, msg_type=%d, len=%d, size=%d\n", msg_id, msg_type, len, outbox_get_size(outbox));
    return item;
}

outbox_item_handle_t outbox_get(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        if (item->msg_id == msg_id) {
            return item;
        }
    }
    return NULL;
}

outbox_item_handle_t outbox_dequeue(outbox_handle_t outbox)
{
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        if (!item->pending) {
            return item;
        }
    }
    return NULL;
}
int outbox_delete(outbox_handle_t outbox, int msg_id, int msg_type)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_id == msg_id && item->msg_type == msg_type) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            mqtt_free(item->buffer);
            mqtt_free(item);
            os_printf(LM_APP, LL_INFO, "MQTT:DELETED msgid=%d, msg_type=%d, remain size=%d\n", msg_id, msg_type, outbox_get_size(outbox));
            return 0;
        }

    }
    return -1;
}
int outbox_delete_msgid(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_id == msg_id) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            mqtt_free(item->buffer);
            mqtt_free(item);
        }

    }
    return 0;
}

int outbox_set_pending(outbox_handle_t outbox, int msg_id)
{
    outbox_item_handle_t item = outbox_get(outbox, msg_id);
    if (item) {
        item->pending = BOOL_TRUE;
        return 0;
    }
    return -1;
}

int outbox_delete_msgtype(outbox_handle_t outbox, int msg_type)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (item->msg_type == msg_type) {
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            mqtt_free(item->buffer);
            mqtt_free(item);
        }

    }
    return 0;
}

int outbox_delete_expired(outbox_handle_t outbox, int current_tick, int timeout)
{
    outbox_item_handle_t item, tmp;
    STAILQ_FOREACH_SAFE(item, outbox, next, tmp) {
        if (current_tick - item->tick > timeout) { // consider overflow
            STAILQ_REMOVE(outbox, item, outbox_item, next);
            mqtt_free(item->buffer);
            mqtt_free(item);
        }

    }
    return 0;
}

int outbox_get_size(outbox_handle_t outbox)
{
    int siz = 0;
    outbox_item_handle_t item;
    STAILQ_FOREACH(item, outbox, next) {
        siz += item->len;
    }
    return siz;
}

int outbox_cleanup(outbox_handle_t outbox, int max_size)
{
    while(outbox_get_size(outbox) > max_size) {
        outbox_item_handle_t item = outbox_dequeue(outbox);
        if (item == NULL) {
            return -1;
        }
        STAILQ_REMOVE(outbox, item, outbox_item, next);
        mqtt_free(item->buffer);
        mqtt_free(item);
    }
    return 0;
}

void outbox_destroy(outbox_handle_t outbox)
{
    outbox_cleanup(outbox, 0);
    mqtt_free(outbox);
}
