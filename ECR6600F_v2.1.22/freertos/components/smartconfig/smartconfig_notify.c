#include "system_wifi.h"
#include "smartconfig.h"
#include "smartconfig_notify.h"
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Flag to indicate sending smartconfig notify or not. */
static bool g_sc_notify_send = false;

static int sc_notify_send_get_errno(int fd)
{
    int sock_errno = 0;
    u32_t optlen = sizeof(sock_errno);

    getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen);

    return sock_errno;
}

static void sc_notify_send_task(void *pvParameters)
{
    sc_notify_t *msg = (sc_notify_t *)pvParameters;
    struct sockaddr_in server_addr;
    int send_sock = -1;
    int optval;
    int sendlen;
    uint8_t packet_count = 0;
    int err;

    os_memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    server_addr.sin_port = htons(SC_NOTIFY_SERVER_PORT);
    wifi_get_mac_addr(STATION_IF, (uint8_t*)msg->mac);
    if (g_sc_notify_send) {
        /* Create UDP socket. */
        send_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (send_sock < 0) {
            sc_debug("Creat udp socket failed\n");
            goto _end;
        }
        optval = 1;
        setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR, &optval, sizeof(int));
        sc_info("%02x %02x %02x %02x %02x %02x %02x\n", msg->random,
            msg->mac[0], msg->mac[1], msg->mac[2], msg->mac[3], msg->mac[4], msg->mac[5]);
    }

    while (g_sc_notify_send) {
        /* Send smartconfig notify message every 100ms. */
        os_msleep(500);
        sendlen = sendto(send_sock, msg, SC_NOTIFY_MSG_LEN, 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
        if (sendlen > 0) {
            /* Totally send 30 message. */
            if (packet_count++ > SC_NOTIFY_MAX_COUNT) {
                goto _end;
            }
        } else {
            err = sc_notify_send_get_errno(send_sock);
            if (err == ENOMEM || err == EAGAIN) {
                sc_debug("send failed, errno %d\n", err);
                continue;
            }
            sc_debug("send notify sucess, errno %d\n", err);
            goto _end;
        }
    }
    os_msleep(100);


_end:
    if (send_sock >= 0) {
        close(send_sock);
    }
    os_free(msg);
    vTaskDelete(NULL);
}

void sc_notify_send(sc_notify_t *param)
{
    sc_notify_t *msg = NULL;

    if (param == NULL) {
        os_printf(LM_WIFI, LL_ERR, "Smart config notify parameter error\n");
        return;
    }

    msg = os_malloc(sizeof(sc_notify_t));
    if (msg == NULL) {
        os_printf(LM_WIFI, LL_ERR, "Smart config notify parameter malloc fail");
        return;
    }
    os_memcpy(msg, param, sizeof(sc_notify_t));

    g_sc_notify_send = true;

    if (xTaskCreate(sc_notify_send_task, "sc_notify_send_task", SC_NOTIFY_TASK_STACK_SIZE, msg, SC_NOTIFY_TASK_PRIORITY, NULL) != pdPASS) {
        os_printf(LM_WIFI, LL_ERR, "Create sending smartconfig notify task fail");
        os_free(msg);
    }
}

void sc_notify_send_stop(void)
{
    g_sc_notify_send = false;
}
