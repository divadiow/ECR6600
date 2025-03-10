/**
 * @file cmd_iperf.c
 * @brief This is a brief description
 * @details This is the detail description
 * @author liuyong
 * @date 2021-6-9
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

#include "cli.h"
#include "iperf.h"
#include "iperf_api.h"

#define IPERF_LINK_MAX_NUM 16
#define IPERF_LINK_SERVER 4
#define IPERF_LINK_CLIENT 16

struct iperf_test *iperf_thread[IPERF_LINK_MAX_NUM];
extern int iperf3_main(int argc , char *argv[]);

static int iperf_thread_dump(void)
{
    int index;
    struct iperf_test *test;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        test = iperf_thread[index];
        if (test == NULL)
            continue;

        if (test->role == 's')
            printf("server->role %c port=%d state=%d snum=%d\n", test->role, test->server_port, test->state, test->num_streams);
        else
            printf("server->role %c ip %s port=%d state=%d snum=%d\n", test->role, test->server_hostname, test->server_port, test->state, test->num_streams);
    }

    return CMD_RET_SUCCESS;
}

static int iperf_stop_server(int port)
{
    int index;
    struct iperf_test *server;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        server = iperf_thread[index];
        if (server == NULL)
            continue;

        if (server->role == 's' && server->server_port == port) {
            server->state = IPERF_DONE;
            server->one_off = 1;
            return CMD_RET_SUCCESS;
        }
    }

    printf("server not start with port %d\n", port);
    return CMD_RET_FAILURE;
}

static int iperf_stop_client(char *ip, int port)
{
    int index;
    struct iperf_test *client;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        client = iperf_thread[index];
        if (client == NULL)
            continue;

        if (client->role == 'c' && client->server_port == port && !strcmp(client->server_hostname, ip)) {
            if (client->state != IPERF_DONE) {
                client->state = IPERF_DONE;
                if (client->state == TEST_RUNNING) {
                    if (iperf_set_send_state(client, TEST_END) != 0)
                        return CMD_RET_FAILURE;
                }
            }
            return CMD_RET_SUCCESS;
        }
    }

    printf("client not start with ip %s port %d\n", ip, port);
    return CMD_RET_FAILURE;
}

int iperf_sc_check(struct iperf_test *sc)
{
    int index;
    int tcpnum = 0;
    int udpnum = 0;
    int snum = 0;

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        if (iperf_thread[index] == NULL)
            continue;

        if (sc->role == 'c')
        {
            if (sc != iperf_thread[index] && sc->server_port == iperf_thread[index]->server_port && !strcmp(sc->server_hostname, iperf_thread[index]->server_hostname))
            {
                printf("client donot start same ip %s port %d\n", sc->server_hostname, sc->server_port);
                return CMD_RET_FAILURE;
            }
        }

        if (sc->role == 's')
        {
            if (sc != iperf_thread[index] && iperf_thread[index]->role == 's' && sc->server_port == iperf_thread[index]->server_port)
            {
                printf("server donot start same port %d\n", sc->server_port);
                return CMD_RET_FAILURE;
            }
        }

        if (iperf_thread[index]->role == 's')
        {
            if ((sc->state > 0) && sc->state != IPERF_START)
            {
                tcpnum += 1;
            }
            snum += 1;
            if (snum > IPERF_LINK_SERVER)
            {
                printf("server max link, please check...\n");
                return CMD_RET_FAILURE;
            }
        }
        else
        {
            tcpnum += 1;
        }

        if (iperf_thread[index]->protocol->id == SOCK_DGRAM)
        {
            udpnum += iperf_thread[index]->num_streams;
        }
        else
        {
            tcpnum += iperf_thread[index]->num_streams;
        }

        if (udpnum + tcpnum > IPERF_LINK_CLIENT || udpnum > IPERF_LINK_CLIENT/2)
        {
            printf("iperf max link, please check...\n");
            return CMD_RET_FAILURE;
        }
    }

    return CMD_RET_SUCCESS;
}

int iperf3_test(cmd_tbl_t *t, int argc, char *argv[])
{
    int index;
    int port = PORT;
    char taskname[32] = {0};

    if (argc <= 1) {
        return CMD_RET_FAILURE;
    }

    if (strcmp(argv[1], "stop") == 0)
    {
        //iperf stop port xxx
        if (strcmp(argv[2], "port") == 0)
        {
            if (argc == 4)
                port = atoi(argv[3]);
            return iperf_stop_server(port);
        }

        if (strcmp(argv[2], "ip") == 0)
        {
            if (strcmp(argv[4], "port") == 0)
            {
                port = atoi(argv[5]);
            }
            return iperf_stop_client(argv[3], port);
        }

        os_printf(LM_CMD, LL_INFO, "iperf stop port[ip] xxx\n");
        return CMD_RET_FAILURE;
    }

    if (strcmp(argv[1], "print") == 0)
    {
        return iperf_thread_dump();
    }

    if (argc == 2 && !strcmp(argv[1], "-h"))
    {
        iperf3_main(argc, argv);
        return CMD_RET_SUCCESS;
    }

    for (index = 0; index < IPERF_LINK_MAX_NUM; index++)
    {
        if (iperf_thread[index] == NULL)
        {
            iperf_thread[index] = iperf_new_test();
            if (!iperf_thread[index])
            {
                printf("iperf_new_test failed\n");
                return CMD_RET_FAILURE;
            }

            iperf_defaults(iperf_thread[index]);
            if (iperf_parse_arguments(iperf_thread[index], argc, argv) < 0)
            {
                printf("iperf_parse_arguments failed\n");
                iperf_free_test(iperf_thread[index]);
                iperf_thread[index] = NULL;
                return CMD_RET_SUCCESS;
            }

            if (!iperf_sc_check(iperf_thread[index]))
            {
                snprintf(taskname, sizeof(taskname), "iperf_%c_%d", iperf_thread[index]->role, iperf_thread[index]->server_port);
                os_task_create(taskname, LWIP_IPERF_TASK_PRIORITY, LWIP_IPERF_TASK_STACK_SIZE, iperf3_thread, &iperf_thread[index]);
                printf("iperf %c port %d start\n",  iperf_thread[index]->role, iperf_thread[index]->server_port);
                return CMD_RET_SUCCESS;
            } else {
                iperf_free_test(iperf_thread[index]);
                iperf_thread[index] = NULL;
	    }

            return CMD_RET_FAILURE;
        }
    }

    printf("max link start\n");
    return CMD_RET_FAILURE;
}


CLI_CMD(iperf, iperf3_test, "iperf3 cli", "iperf3 test cli");



