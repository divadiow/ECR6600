#include "spi.h"
#include <string.h>
#include "chip_pinmux.h"
#include <stdarg.h>
#include "cli.h"
os_sem_handle_t spi_slave_rx_process;

static int spi_init_test(cmd_tbl_t *t, int argc, char *argv[])
{
	spi_interface_config_t spi_master_dev = 
		{	
			.addr_len = 3,
			.data_len = 8,
			.spi_clk_pol = 0,
			.spi_clk_pha = 0,
			.spi_trans_mode=SPI_MODE_STANDARD,
			.master_clk = 10,
			.addr_pha_enable = 0,
			.cmd_read = SPI_TRANSCTRL_TRAMODE_DR|SPI_TRANSCTRL_CMDEN,
			.cmd_write = SPI_TRANSCTRL_TRAMODE_DW|SPI_TRANSCTRL_CMDEN,
			//.cmd_write = SPI_TRANSCTRL_TRAMODE_WO,
			.dummy_bit =SPI_TRANSCTRL_DUMMY_CNT_1,
			.spi_dma_enable =0
		};	
	spi_init_cfg(&spi_master_dev);	

	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_init, spi_init_test, "spi_init_test", "spi_init_test");

static int spi_write_test(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int len;
	len = strtoul(argv[1], NULL, 0);
	spi_transaction_t config;
	config.cmmand = 0x51;
	config.addr = 0x0;
	config.length = len;

	unsigned char test[1024]={0};
	int i;
	for(i=0;i<1024;++i)
	{
		test[i] = (unsigned char)(i%256);

	}
	spi_master_write((unsigned char*)test, &config );

	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_tx, spi_write_test, "test_spi_write", "test_spi_write");

static int spi_read_test(cmd_tbl_t *t, int argc, char *argv[])
{
	spi_transaction_t config;
	config.cmmand = 0x0B;
	config.addr = 0x0;
	config.length = 512;

	unsigned char test[1024]={0};
	spi_master_read((unsigned char*)test,&config );
	int i=0;
	int pass = 1;

	for(i=0;i<config.length;++i)
	{

		if(test[i] != i%256)
		{
			pass = 0;
			os_printf(LM_APP, LL_INFO, "sfTest[%d]=0x%x\n",i,test[i]);
		}
	}

	if(pass==1)
		os_printf(LM_APP, LL_INFO, "PASS\n");
	else
		os_printf(LM_APP, LL_INFO, "FAIL\n");

	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_rx, spi_read_test, "test_spi_read", "test_spi_read");


static int spi_slave_init_test(cmd_tbl_t *t, int argc, char *argv[])
{
	spi_slave_interface_config_t  spi_slave_dev = 
	{			
		.inten = 0x20,
		.addr_len = 3,
		.data_len = 8,
		.spi_clk_pol = 0,
		.spi_clk_pha = 0,
		.slave_clk = 10,
		.spi_slave_dma_enable = 0
	};
	spi_init_slave_cfg(&spi_slave_dev);	
	return CMD_RET_SUCCESS;
}

CLI_CMD(slave_init, spi_slave_init_test, "spi_slave_init", "spi_slave_init");



static int spi_slave_write_test(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned char test[512]={0};
	int i;
	for(i=0;i<512;++i)
	{
		test[i] = (unsigned char)(i%256);

	}
	spi_slave_write((unsigned char*)test, 512 );
	return CMD_RET_SUCCESS;
}

CLI_CMD(slave_tx, spi_slave_write_test, "test_spi_slave_write", "test_spi_slave_write");



void spi_slave_read_task()
{
	 while(1)
	 {
	 	unsigned char test[512]={0};
	 	os_sem_wait(spi_slave_rx_process, 0xffffffff);
		spi_slave_read((unsigned char *)test,512);
		int ok = 1;
		int  i;
		for(i=0;i<512;++i)
		{
			if(((unsigned char)(test[i]))!=(unsigned char)(i%256))
			{
				ok = 0;
				os_printf(LM_APP, LL_INFO, "test[%d]=%d\n",i,test[i]);
			}
		}
		if(ok==1)
			os_printf(LM_APP, LL_INFO, "PASS\n");
		else
			os_printf(LM_APP, LL_INFO, "FAIL\n");
	 }
}


static int spi_slave_read_test(cmd_tbl_t *t, int argc, char *argv[])
{
	#if 1

	//	cpu test
	unsigned char test[512]={0};
	int i;
	spi_slave_read((unsigned char *)test,512);
	int ok = 1;
	for(i=0;i<512;++i)
	{
		if(((unsigned char)(test[i]))!=(unsigned char)(i%256))
		{
			ok = 0;
			os_printf(LM_APP, LL_INFO, "test[%d]=%d\n",i,test[i]);
		}
	}
	if(ok==1)
		os_printf(LM_APP, LL_INFO, "PASS\n");
	else
		os_printf(LM_APP, LL_INFO, "FAIL\n");
	#else
	//dma test
	os_task_create("spi_slave_read_task",4,4096,spi_slave_read_task,NULL);

	spi_slave_rx_dma_trans();
	#endif

	return CMD_RET_SUCCESS;
}

CLI_CMD(slave_rx, spi_slave_read_test, "test_spi_slave_read", "test_spi_slave_read");



static int spi_test(cmd_tbl_t *t, int argc, char *argv[])
{	
	os_printf(LM_APP, LL_INFO, "new_len =%d\n", new_len);
	os_printf(LM_APP, LL_INFO, "remain_read =%d\n", remain_read * 4);
	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_a, spi_test, "spi_slave_init", "spi_slave_init");

