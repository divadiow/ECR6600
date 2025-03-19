#include "spi_slave_zeratul.h"
#include "cli.h"

extern void zeratul_spi_init(void);

static int spi_slave_init_test(cmd_tbl_t *t, int argc, char *argv[])
{
    zeratul_spi_init();

    return CMD_RET_SUCCESS;
}

CLI_CMD(slave_init, spi_slave_init_test, "spi_slave_init_test", "spi_slave_init_test");
