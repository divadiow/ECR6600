/**
 * @file tkl_spi.c
 * @brief bt操作接口
 * 
 * @copyright Copyright (c) {2018-2022} 涂鸦科技 www.tuya.com
 * 
 */
#include "spi.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
//#include "arch_irq.h"
#include <string.h>

#include "tkl_spi.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tkl_mutex.h"
#include "cli.h"

/*============================ MACROS ========================================*/

/*============================ TYPES =========================================*/

static char spi_inited = 0;
static TKL_MUTEX_HANDLE spi_mutex = NULL;
/**
 * @brief spi init
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_BASE_CFG_T *cfg)
{
	spi_interface_config_t spi_master_dev;
	spi_slave_interface_config_t  spi_slave_dev;
	unsigned int feq;

	if (NULL == cfg) {
		return OPRT_INVALID_PARM;
	}

	if (spi_inited) {
		return OPRT_OK;
	}

	memset(&spi_master_dev, 0, sizeof((spi_master_dev)));
	memset(&spi_slave_dev, 0, sizeof((spi_slave_dev)));

	switch (cfg->mode) {
	case TUYA_SPI_MODE1:
	{
		spi_master_dev.spi_clk_pol = 0;
		spi_master_dev.spi_clk_pha = 1;
		spi_slave_dev.spi_clk_pol = 0;
		spi_slave_dev.spi_clk_pha = 1;
		break;
	}
	case TUYA_SPI_MODE2:
	{
		spi_master_dev.spi_clk_pol = 1;
		spi_master_dev.spi_clk_pha = 0;
		spi_slave_dev.spi_clk_pol = 1;
		spi_slave_dev.spi_clk_pha = 0;
		break;
	}
	case TUYA_SPI_MODE3:
	{
		spi_master_dev.spi_clk_pol = 1;
		spi_master_dev.spi_clk_pha = 1;
		spi_slave_dev.spi_clk_pol = 1;
		spi_slave_dev.spi_clk_pha = 1;
		break;
	}
	default:
	{
		spi_master_dev.spi_clk_pol = 0;
		spi_master_dev.spi_clk_pha = 0;
		spi_slave_dev.spi_clk_pol = 0;
		spi_slave_dev.spi_clk_pha = 0;
		break;
	}
	} /* End switch (cfg->mode) { */

	if (cfg->spi_dma_flags) {
		spi_master_dev.spi_dma_enable = 1;
		spi_slave_dev.spi_slave_dma_enable = 1;
	}

	spi_master_dev.master_clk = 9; /* 2M ~  1M */
	spi_slave_dev.slave_clk = 9; /* 2M ~  1M */

	/* bitorder TUYA_SPI_ORDER_MSB2LSB */
#if 1
	if (cfg->freq_hz < 1000000) {
		spi_master_dev.master_clk = 19;
	} else {
		feq = (cfg->freq_hz + 1000000 - 1) / 1000000;
		if (feq >= 20) {
			spi_master_dev.master_clk = 0;
		} else if (feq >= 10) {
			spi_master_dev.master_clk = 1;
		} else if (feq >= 6) {
			spi_master_dev.master_clk = 2;
		} else if (feq >= 5) {
			spi_master_dev.master_clk = 3;
		} else if (feq >= 4) {
			spi_master_dev.master_clk = 4;
		} else if (feq >= 2) {
			spi_master_dev.master_clk = 9;
		} else if (feq >= 1) {
			spi_master_dev.master_clk = 19;
		}
	}
#endif	
	if (TUYA_SPI_ROLE_MASTER == cfg->role) {
		spi_master_dev.addr_len = 3;
		if (TUYA_SPI_DATA_BIT16 == cfg->databits) {
			spi_master_dev.data_len = 16;
		} else {
			spi_master_dev.data_len = 8;
		}
		spi_master_dev.spi_trans_mode = SPI_MODE_STANDARD;
		spi_master_dev.addr_pha_enable = 0;
		spi_master_dev.cmd_read = SPI_TRANSCTRL_TRAMODE_RO|SPI_TRANSCTRL_CMDDIS;
		spi_master_dev.cmd_write = SPI_TRANSCTRL_TRAMODE_WO|SPI_TRANSCTRL_CMDDIS;
		spi_master_dev.dummy_bit = SPI_TRANSCTRL_DUMMY_CNT_1;
#if 1
		os_printf(LM_APP, LL_INFO, "pol %d pha %d  dma %d freq_hz %u master_clk %d data_len %d\r\n",
			spi_master_dev.spi_clk_pol, spi_master_dev.spi_clk_pha, spi_master_dev.spi_dma_enable,
			cfg->freq_hz, spi_master_dev.master_clk, spi_master_dev.data_len);
#endif
		if (spi_init_cfg(&spi_master_dev) < 0) {
			//os_printf(LM_APP, LL_INFO, "%s: call spi_init_cfg failed\r\n", __func__);
			return OPRT_COM_ERROR;
		}
	} else if (TUYA_SPI_ROLE_SLAVE == cfg->role) {
		spi_slave_dev.inten = 0x20;
		spi_slave_dev.addr_len = 3;
		if (TUYA_SPI_DATA_BIT16 == cfg->databits) {
			spi_slave_dev.data_len = 16;
		} else {
			spi_slave_dev.data_len = 8;
		}

		if (spi_init_slave_cfg(&spi_slave_dev) < 0) {
			return OPRT_COM_ERROR;
		}
	} else {
		return OPRT_NOT_SUPPORTED;
	}

	if (NULL == spi_mutex) {
		if (tkl_mutex_create_init(&spi_mutex) < 0) {
			//os_printf(LM_APP, LL_INFO,"%s: call tkl_mutex_create_init failed\r\n", __func__);
			return OPRT_COM_ERROR;
		}
	}

	spi_inited = 1;
	return OPRT_OK;
}

/**
 * @brief spi deinit
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
	if (NULL != spi_mutex) {
		tkl_mutex_release(spi_mutex);
		spi_mutex = NULL;
	}
	drv_spi_master_close();
	spi_inited = 0;

	//os_printf(LM_APP, LL_INFO,"%s: port %d\r\n", __func__, port);
	return OPRT_OK;
}

/**
 * Spi send
 *
 * @param[in]  port      the spi device
 * @param[in]  data     spi send data
 * @param[in]  size     spi send data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
#define MAX_TY_SPI_SEND_BUF_LEN 512
	unsigned char *pointer;
	spi_transaction_t config;

	config.cmmand = 0x51;
	config.addr = 0x33445566;
	config.length = size;

	pointer = (unsigned char *)data;
	tkl_mutex_lock(spi_mutex);
#if 0
	os_printf(LM_APP, LL_INFO, "%s: size %d\r\n", __func__, size);
	unsigned char *p = (unsigned char *)pointer;
	for (int i = 0; i < size; i++) {
		os_printf(LM_APP, LL_INFO, " %02x", p[i]);
	}
	os_printf(LM_APP, LL_INFO,"\r\n");
#endif

	if (spi_master_write((unsigned char *)pointer, &config) < 0) {
		tkl_mutex_unlock(spi_mutex);
		//os_printf(LM_APP, LL_INFO,"%s: call spi_master_write failed\r\n", __func__);
		return OPRT_COM_ERROR;
	}

	tkl_mutex_unlock(spi_mutex);
	return OPRT_OK;
}

/**
 * spi_recv
 *
 * @param[in]   port      the spi device
 * @param[out]  data     spi recv data
 * @param[in]   size     spi recv data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
	spi_transaction_t config;

	config.cmmand = 0x0B;
	config.addr = 0x0;
	config.length = size;

	tkl_mutex_lock(spi_mutex);
	
	//os_printf(LM_APP, LL_INFO,"%s: size %d\r\n", __func__, size);
	if (spi_master_read((unsigned char*)data, &config)) {
		tkl_mutex_unlock(spi_mutex);
		//os_printf(LM_APP, LL_INFO,"%s: call spi_master_read failed\r\n");
		return OPRT_COM_ERROR;
	}
#if 0
	unsigned char *p = (unsigned char *)data;
	for (int i = 0; i < size; i++) {
		os_printf(LM_APP, LL_INFO, " %02x", p[i]);
	}
	os_printf(LM_APP, LL_INFO,"\r\n");
#endif
	tkl_mutex_unlock(spi_mutex);
	return OPRT_OK;
}

/**
 * @brief spi transfer
 * 
 * @param[in] port: spi port
 * @param[in] send_buf: spi send buf
 * @param[out] send_buf:spi recv buf
 * @param[in] length: spi msg length
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, VOID_T* send_buf, VOID_T* receive_buf, UINT32_T length)
{
	tkl_mutex_lock(spi_mutex);

	if (spi_master_write_read((unsigned char *)send_buf, (unsigned char *)receive_buf, length, 1) < 0) {
		tkl_mutex_unlock(spi_mutex);
		return OPRT_COM_ERROR;
	}

	tkl_mutex_unlock(spi_mutex);
	return OPRT_OK;
}

/**
 * @brief adort spi transfer,or spi send, or spi recv
 * 
 * @param[in] port: spi port
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief get spi status.
 * 
 * @param[in] port: spi port
 * @param[out]  TUYA_SPI_STATUS_T,please refer to tuya_cloud_types.h
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: spi port, id index starts at 0
 * @param[in] cb:  spi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi irq enable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi irq disable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi transferred data count.
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return >=0,number of currently transferred data items. <0,err. 
 * during  tkl_spi_send, tkl_spi_recv and tkl_spi_transfer operation.
 */
INT32_T tkl_spi_get_data_count(TUYA_SPI_NUM_E port)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi ioctl
 *
 * @param[in]       cmd     user def
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_ioctl(TUYA_SPI_NUM_E port, UINT32_T cmd,  VOID *args)
{
	return OPRT_NOT_SUPPORTED;
}

#if 1
static int spi_ty_init_test(cmd_tbl_t *t, int argc, char *argv[])
{
	TUYA_SPI_BASE_CFG_T cfg;

	memset(&cfg, 0, sizeof(cfg));

	cfg.bitorder = 0;
	cfg.databits = 0;
	cfg.freq_hz = 6600000;
	cfg.spi_dma_flags = 0;
	cfg.role = TUYA_SPI_ROLE_MASTER;
	cfg.mode = TUYA_SPI_MODE0;
	cfg.type = TUYA_SPI_AUTO_TYPE;

	if (tkl_spi_init(0, &cfg) < 0) {
		os_printf(LM_APP, LL_INFO,"%s: call tkl_spi_init failed\r\n", __func__);
	}

	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_ty_init, spi_ty_init_test, "spi_ty_init_test", "spi_ty_init_test");


static int spi_ty_write_test( int argc, char *argv[])
{
#define TY_SPI_TEST_BUF_LEN 4800
	unsigned char *test = (unsigned char *)tkl_system_malloc(TY_SPI_TEST_BUF_LEN);
	if (NULL == test) {
		return OPRT_MALLOC_FAILED;
	}

	int i;
	for(i=0;i<TY_SPI_TEST_BUF_LEN;++i)
	{
		test[i] = (unsigned char)(i%256);
	}

	if (tkl_spi_send(0, test, TY_SPI_TEST_BUF_LEN) < 0) {
		os_printf(LM_APP, LL_INFO,"%s: call tkl_spi_send failed\r\n", __func__);
	}

	tkl_system_free(test);
	return CMD_RET_SUCCESS;
}

CLI_CMD(spi_ty_tx, spi_ty_write_test, "test_ty_spi_write", "test_ty_spi_write");

#endif
