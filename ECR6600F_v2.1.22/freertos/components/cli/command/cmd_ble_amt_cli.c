/*******************************************************************************
 * Copyright by eswin.
 *
 * File Name: ble_amt_cli.c
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v1.0
 * Author:        xulei
 * Date:         
 ********************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "cmd_ble_amt_cli.h"
//#include "hal_rf_6600.h"
#include "cli.h"
#include "amtNV.h"
#include "uart.h"
#include "tx_power_config.h"
#ifdef NX_ESWIN_SDIO
#include "amt_transport.h"
#endif
extern int temp_status;
//#ifndef WRITE_REG
//#define WRITE_REG(offset,value) (*(volatile unsigned int *)(offset) = (unsigned int)(value));
//#endif
//#ifndef READ_REG
//#define READ_REG(reg) (  *( (volatile unsigned int *) (reg) ) )
//#endif

#define REG_PL_WR(addr, value)       (*(volatile uint32_t *)(addr)) = (value)
#define REG_PL_RD(addr)             (*(volatile uint32_t *)(addr))

//#if defined(AMT) && defined(CONFIG_NV)
/* AMT test command */

#define CMD_TEST_MODE_LEN (13)
#define CMD_TEST_MODE_STOP_LEN (4)
#define CMD_TEST_MODE_RESET_LEN (4)

static uint8_t cmd_tm[CMD_TEST_MODE_LEN]={0};
static uint8_t cmd_tm_stop[CMD_TEST_MODE_STOP_LEN]={0x01,0x1f,0x20,0x00};
static uint8_t cmd_tm_reset[CMD_TEST_MODE_RESET_LEN]={0x01,0x03,0x0c,0x00};
static uint8_t dc_flag = 0;

extern void uart_buff_cpy_for_amt(char *cmd, int cmd_len);
static int AmtBleTxStart(cmd_tbl_t *t, int argc, char *argv[])
{
	#if defined(CONFIG_MULTIPLEX_UART)
    uint8_t phy, ch, pkt_type, pdu_len, gain_value;
	uint8_t tx[4] = {0x01,0x50,0x20,0x09};
	uint8_t remain[5] = {0x00,0x00,0x02,0x00,0x00};

	if(argc < 6)
	{
		return CMD_RET_FAILURE;
	}

    phy			= 	atoi(argv[1]);
    ch			= 	atoi(argv[2]);
    pkt_type	= 	atoi(argv[3]);
    pdu_len		= 	atoi(argv[4]);
	gain_value	= 	atoi(argv[5]);

	//64+14*（ch-39）/39+0.5
	float di_gain1 = (float)(0.5+((ble_tx_power_index[0]>>8)&0x1F)+((ble_tx_power_index[0]>>13)&0x7) * (ch - 39) /39);
	float di_gain2 = (float)(0.5+((ble_tx_power_index[1]>>8)&0x1F)+((ble_tx_power_index[1]>>13)&0x7) * (ch - 39) /39);
	float di_gain3 = (float)(0.5+((ble_tx_power_index[2]>>8)&0x1F)+((ble_tx_power_index[2]>>13)&0x7) * (ch - 39) /39);
	
	switch(gain_value)
	{
		case LOW_LEVEL:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain1&0xff)<<8) + (ble_tx_power_index[0]&0xff) ); //6dbm
			break;
		case AVERAGE_LEVEL:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain2&0xff)<<8) + (ble_tx_power_index[1]&0xff) ); //8dbm
			break;
		default:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain3&0xff)<<8) + (ble_tx_power_index[2]&0xff) ); //10dbm
			break;
	}
	WRITE_REG(0X2031D0, 1);

	memcpy(cmd_tm, tx, 4);

	cmd_tm[4]=ch-1;
	cmd_tm[5]=pdu_len;

	switch(pkt_type)
	{
		case T_PRBS9:
			cmd_tm[6]=0x00;
			break;
		case T_1100:
			cmd_tm[6]=0x01;
			break;
		case T_1010:
			cmd_tm[6]=0x02;
			break;
		case T_PRBS15:
			cmd_tm[6]=0x03;
			break;
		case T_1111:
			cmd_tm[6]=0x04;
			break;
		case T_0000:
			cmd_tm[6]=0x05;
			break;
		case T_0011:
			cmd_tm[6]=0x06;
			break;
		case T_0101:
			cmd_tm[6]=0x07;
			break;
		default:
			cmd_tm[6]=0x00;
			break;
	}

	switch(phy)
	{
		case UNCODE_PHY_1M:
			cmd_tm[7]=0x01;
			break;
		case UNCODE_PHY_2M:
			cmd_tm[7]=0x02;
			break;
		case CODE_PHY_S2:
			cmd_tm[7]=0x03;
			break;
		default:
			cmd_tm[7]=0x04;
			break;
	}

	memcpy(&cmd_tm[8], remain, 5);

	uart_buff_cpy_for_amt((char *)cmd_tm, CMD_TEST_MODE_LEN);
	#endif

    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleTxStart, AmtBleTxStart, "amtBleTxStart", "amtBleTxStart", E_AMT);

static int AmtBleTxStop(cmd_tbl_t *t, int argc, char *argv[])
{
    if (dc_flag == 1)
    {
        REG_PL_WR(0x00760000, 0);
        REG_PL_WR(0x20e000, 0);
    }

    #if defined(CONFIG_MULTIPLEX_UART)
	uart_buff_cpy_for_amt((char *)cmd_tm_stop, CMD_TEST_MODE_STOP_LEN);
	#endif

    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleTxStop, AmtBleTxStop, "amtBleTxStop", "amtBleTxStop", E_AMT);

static int AmtBleRxStart(cmd_tbl_t *t, int argc, char *argv[])
{
	#if defined(CONFIG_MULTIPLEX_UART)
    uint8_t phy, ch;
	uint8_t rx[4] = {0x01,0x4f,0x20,0x09};
	uint8_t remain[7] = {0x00,0x00,0x00,0x01,0x02,0x00,0x00};
	
    phy			=  atoi(argv[1]);
    ch			=  atoi(argv[2]);

	memcpy(cmd_tm, rx, 4);

	cmd_tm[4]=ch-1;

	switch(phy)
	{
		case UNCODE_PHY_1M:
			cmd_tm[5]=0x01;
			break;
		case UNCODE_PHY_2M:
			cmd_tm[5]=0x02;
			break;
		case CODE_PHY_S2:
			cmd_tm[5]=0x03;
			break;
		default:
			cmd_tm[5]=0x04;
			break;
	}

	memcpy(&cmd_tm[6], remain, 7);

	uart_buff_cpy_for_amt((char *)cmd_tm, CMD_TEST_MODE_LEN);
	#endif

    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleRxStart, AmtBleRxStart, "amtBleRxStart", "amtBleRxStart", E_AMT);

static int AmtBleRxStop(cmd_tbl_t *t, int argc, char *argv[])
{
    #if defined(CONFIG_MULTIPLEX_UART) && defined(NX_ESWIN_SDIO)
	int32_t rx_num = READ_REG(0x4600d8);
	uart_buff_cpy_for_amt((char *)cmd_tm_stop, CMD_TEST_MODE_STOP_LEN);
	AMT_OUTPUT("%d,%s", rx_num, CMD_RET_OK);
	#endif
	os_printf(LM_APP, LL_INFO, "%ld,", READ_REG(0x4600d8));
    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleRxStop, AmtBleRxStop, "amtBleRxStop", "amtBleRxStop", E_AMT);

static int AmtBleReset(cmd_tbl_t *t, int argc, char *argv[])
{
	#if defined(CONFIG_MULTIPLEX_UART)
	uart_buff_cpy_for_amt((char *)cmd_tm_reset, CMD_TEST_MODE_RESET_LEN);
	#endif
	return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleReset, AmtBleReset, "amtBleReset", "amtBleReset", E_AMT);


static int AmtBleTxToneStart(cmd_tbl_t *t, int argc, char *argv[])
{
	#if defined(CONFIG_MULTIPLEX_UART)
    uint8_t phy, ch, pkt_type, pdu_len, gain_value;
	uint8_t tx[4] = {0x01,0x60,0x20,0x09};
	uint8_t remain[5] = {0x00,0x00,0x02,0x00,0x00};

	if(argc < 4)
	{
		return CMD_RET_FAILURE;
	}

    phy			= 	atoi(argv[1]);
    ch			= 	atoi(argv[2]);
    pkt_type	= 	0x04;
    pdu_len		= 	37;
	gain_value	= 	atoi(argv[3]);

		//64+14*（ch-39）/39+0.5
	float di_gain1 = (float)(0.5+((ble_tx_power_index[0]>>8)&0x1F)+((ble_tx_power_index[0]>>13)&0x7) * (ch - 39) /39);
	float di_gain2 = (float)(0.5+((ble_tx_power_index[1]>>8)&0x1F)+((ble_tx_power_index[1]>>13)&0x7) * (ch - 39) /39);
	float di_gain3 = (float)(0.5+((ble_tx_power_index[2]>>8)&0x1F)+((ble_tx_power_index[2]>>13)&0x7) * (ch - 39) /39);
	
	switch(gain_value)
	{
		case LOW_LEVEL:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain1&0xff)<<8) + (ble_tx_power_index[0]&0xff) ); //6dbm
			break;
		case AVERAGE_LEVEL:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain2&0xff)<<8) + (ble_tx_power_index[1]&0xff) ); //8dbm
			break;
		default:
			WRITE_REG(0X2031B0, (((uint16_t)di_gain3&0xff)<<8) + (ble_tx_power_index[2]&0xff) ); //10dbm
			break;
	}
	WRITE_REG(0X2031D0, 1);

	memcpy(cmd_tm, tx, 4);

	cmd_tm[4]=ch-1;
	cmd_tm[5]=pdu_len;

	switch(pkt_type)
	{
		case T_PRBS9:
			cmd_tm[6]=0x00;
			break;
		case T_1100:
			cmd_tm[6]=0x01;
			break;
		case T_1010:
			cmd_tm[6]=0x02;
			break;
		case T_PRBS15:
			cmd_tm[6]=0x03;
			break;
		case T_1111:
			cmd_tm[6]=0x04;
			break;
		case T_0000:
			cmd_tm[6]=0x05;
			break;
		case T_0011:
			cmd_tm[6]=0x06;
			break;
		case T_0101:
			cmd_tm[6]=0x07;
			break;
		default:
			cmd_tm[6]=0x00;
			break;
	}

	switch(phy)
	{
		case UNCODE_PHY_1M:
			cmd_tm[7]=0x01;
			break;
		case UNCODE_PHY_2M:
			cmd_tm[7]=0x02;
			break;
		case CODE_PHY_S2:
			cmd_tm[7]=0x03;
			break;
		default:
			cmd_tm[7]=0x04;
			break;
	}

	memcpy(&cmd_tm[8], remain, 5);

	uart_buff_cpy_for_amt((char *)cmd_tm, CMD_TEST_MODE_LEN);
	#endif

    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleTxToneStart, AmtBleTxToneStart, "amtBleTxToneStart [phy] [ch] [tx_gain]", "amtBleTxToneStart [phy] [ch] [tx_gain]", E_AMT);

#if 1
//#if (!CONFIG_BLE_HOST_PRESENT)
static int AmtBleDumpDc(cmd_tbl_t *t, int argc, char *argv[])
{
    dc_flag = 1;

    REG_PL_WR(0x004600D0, (REG_PL_RD(0x004600D0) & ~((uint32_t)0x00008000)) | ((uint32_t)1 << 15));//infinite tx

    uint32_t value = strtol(argv[1], NULL, 0);
    REG_PL_WR(0x20e000, 0x0); 
    //step1:set iram start address and max address
    REG_PL_WR(0x20e014, 0x0);     //dump start address
    REG_PL_WR(0x20e008, 0x0);     //dump max address
    //step2:debug mode select, 0:rx.1:tx,2:phy dump
    REG_PL_WR(0x20e004, 1);       //0,rx.1 tx,2 phy dump
    //step3:rf lna adc gain,dfe_rx_q_data<11:0>,dfe_rx_i_data<11:0>
    REG_PL_WR(0x80000, value);
    //step4:debug mode enable
    REG_PL_WR(0x20e000, 1);

    //WRITE_REG(0x203040, READ_REG(0x203040)&(~0x10001)); //dig_rx_on_tx =1 dif_rx_on_tx = 1;
    WRITE_REG(0x760000, 0x301);//continues mode.

    return CMD_RET_SUCCESS;
}
CLI_CMD_M(amtBleDumpDc, AmtBleDumpDc, "amtBleDumpDc", "amtBleDumpDc", E_AMT);
#endif //(!CONFIG_BLE_HOST_PRESENT)

//#endif


