#include <stdlib.h>
#include "stdio.h"
#include "stdint.h"
#include "cli.h"
#include "http_server_service.h"

extern httpd_handle_t webconfig_main(void);

static httpd_handle_t server;

#if !defined(CONFIG_TENCENT_CLOUD) && !defined(CONFIG_JD_LED)
int _close_r()
{
 return 0;
}
#endif

static int cmd_httpserver(cmd_tbl_t *t, int argc, char *argv[])
{
    if (argc <= 1) {
        os_printf(LM_CMD, LL_INFO,"too few arguments to command\n");
        return CMD_RET_FAILURE;
    }
    if (strcmp(argv[1], "start") == 0) {
        server = webconfig_main();
        if (server != NULL) {
            os_printf(LM_CMD, LL_INFO,"succeed!\n");
            return CMD_RET_SUCCESS;
        }
        os_printf(LM_CMD, LL_INFO,"failed!\n");
        return CMD_RET_FAILURE;
    }else if(strcmp(argv[1], "stop") == 0) {
        int ret;
        ret = (int)httpd_stop(server);
        if (ret == 0) {
            os_printf(LM_CMD, LL_INFO,"succeed!\n");
            return CMD_RET_SUCCESS;
        }
        os_printf(LM_CMD, LL_INFO,"failed!\n");
        return CMD_RET_FAILURE;
    }else {
        return CMD_RET_FAILURE;
    }
        
}
CLI_CMD(httpserver, cmd_httpserver, "Httpserver", "httpserver [arg]");
