#ifndef _BLE_AMT_CLI_H__
#define _BLE_AMT_CLI_H__

enum amt_ble_tx_gain
{
	LOW_LEVEL = 1,
	AVERAGE_LEVEL = 2,
	HIGH_LEVEL = 3,
};

enum amt_ble_phy
{
	UNCODE_PHY_1M = 0,
	UNCODE_PHY_2M = 1,
	CODE_PHY_S2 = 2,
	CODE_PHY_S8 = 3,
};

enum amt_ble_pkt_type
{
	T_PRBS9 = 0,
	T_1100 = 1,
	T_1010 = 2,
	T_PRBS15 = 3,
	T_1111 = 4,
	T_0000 = 5,
	T_0011 = 6,
	T_0101 = 7
};

#endif //_BLE_AMT_CLI_H__

