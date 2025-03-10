
#ifndef _CHIP_CONFIG_INFO_H
#define _CHIP_CONFIG_INFO_H


/* 6600 CHIP  HARDWARE CONFIGURATION INFO */





/* GPIO  CFG  INFO */
#define CHIP_CFG_GPIO_NUM_MAX	26

/* UART  CFG  INFO */
#define CHIP_CFG_UART_FIFO_DEPTH	32

/* ATE   CAL  INFO*/
//word 0
#define ATE_CHIP_VERSION_MASK                0x3
#define ATE_CHIP_VERSION_OFFSET              30
#define ATE_LOT_ID5_MASK                     0x3f
#define ATE_LOT_ID5_OFFSET                   24
#define ATE_LOT_ID5_MASK                     0x3f
#define ATE_LOT_ID5_OFFSET                   24
#define ATE_LOT_ID4_MASK                     0x3f
#define ATE_LOT_ID4_OFFSET                   18
#define ATE_LOT_ID3_MASK                     0x3f
#define ATE_LOT_ID3_OFFSET                   12
#define ATE_LOT_ID2_MASK                     0x3f
#define ATE_LOT_ID2_OFFSET                   6
#define ATE_LOT_ID1_MASK                     0x3f
#define ATE_LOT_ID1_OFFSET                   0

//word 1
#define ATE_CP_TEST_MASK                     0x1
#define ATE_CP_TEST_OFFSET                   31
#define ATE_CP_VER_MASK                      0x1f
#define ATE_CP_VER_OFFSET                    26
/*reserved*/
#define ATE_Y_COORD_MASK                     0xff
#define ATE_Y_COORD_OFFSET                   13
#define ATE_X_COORD_MASK                     0xff
#define ATE_X_COORD_OFFSET                   5
#define ATE_WAFER_NUM_MASK                   0x1f
#define ATE_WAFER_NUM_OFFSET                 0

//word 2
/*reserved*/
#define ATE_BGM_TUNE_R_MASK                  0x7
#define ATE_BGM_TUNE_R_OFFSET                26
#define ATE_BGM_ANA_MASK                     0x1f
#define ATE_BGM_ANA_OFFSET                   21
#define ATE_BGM_DIG_MASK                     0x1f
#define ATE_BGM_DIG_OFFSET                   16
#define ATE_BGM_0P8_TRIM_MASK                0x7
#define ATE_BGM_0P8_TRIM_OFFSET              13
#define ATE_BGM_0P6_TRIM_MASK                0x7
#define ATE_BGM_0P6_TRIM_OFFSET              10
#define ATE_BGM_IZTC_TRIM_MASK               0x1f
#define ATE_BGM_IZTC_TRIM_OFFSET             5
#define ATE_BGM_IPTATCAL_MASK                0x1f
#define ATE_BGM_IPTATCAL_OFFSET              0

//word 3
#define ATE_LPLDO_VTRIM_FUNC_MASK            0xf
#define ATE_LPLDO_VTRIM_FUNC_OFFSET          28
#define ATE_LPLDO_VTRIM_SLEEP_MASK           0xf
#define ATE_LPLDO_VTRIM_SLEEP_OFFSET         24
#define ATE_OP8_BGP_OF_ADC_MASK              0xff
#define ATE_OP8_BGP_OF_ADC_OFFSET            16
#define ATE_TEM_SENSOR_MASK                  0xffff
#define ATE_TEM_SENSOR_OFFSET                0

//word 4
#define ATE_CHIP_TYPE_MASK                   0x1f
#define ATE_CHIP_TYPE_OFFSET                 27
#define ATE_FT_VER_MASK                      0xf
#define ATE_FT_VER_OFFSET                    22
#define ATE_TXGAIN_11_MASK                   0xf
#define ATE_TXGAIN_11_OFFSET                 18
#define ATE_TXGAIN_6_MASK                    0xf                 
#define ATE_TXGAIN_6_OFFSET                  14
#define ATE_TXGAIN_1_MASK                    0xf
#define ATE_TXGAIN_1_OFFSET                  10
/*reserved*/
#define ATE_CP_CRC8_MASK                     0xff
#define ATE_CP_CRC8_OFFSET                   0

//word 5
#define ATE_BLE_TXP0_MASK                    0x3
#define ATE_BLE_TXP0_OFFSET                  30
#define ATE_BLE_TXP19_MASK                   0x3
#define ATE_BLE_TXP19_OFFSET                 28
#define ATE_BLE_TXP39_MASK                   0x3
#define ATE_BLE_TXP39_OFFSET                 26
#define ATE_IRR_TRIM_MASK                    0xffffff
#define ATE_IRR_TRIM_OFFSET                  0
/*reserved*/
#define ATE_CUSTOMER_ID_FLAG_MASK            0x3
#define ATE_CUSTOMER_ID_FLAG_OFFSET          16
#define ATE_CUSTOMER_ID_MASK                 0xffff
#define ATE_CUSTOMER_ID_OFFSET               0

//wrod6
/*reserved*/

//word7
#define ATE_FT_TEST_MASK                     0x1
#define ATE_FT_TEST_OFFSET                   31
/*reserved*/
#define ATE_FT_CRC8_MASK                     0xff
#define ATE_FT_CRC8_OFFSET                   22
/*reserved*/
#define ATE_MACID_FLAG_MASK                  0x3
#define ATE_MACID_FLAG_OFFSET                16
#define ATE_MACID_MASK                       0xff          
#define ATE_MACID_OFFSET                     0


//sturct define for ate calibration info
#define ATE_CAL_MAX_WORD_LEN     8
typedef struct {
    unsigned int data[ATE_CAL_MAX_WORD_LEN];
}ate_cal_info_t;

extern ate_cal_info_t ate_efuse_info;
void init_ate_efuse_info(void);
void dump_ate_efuse_info(void);

static inline unsigned char chip_bgm_tune_r_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_TUNE_R_MASK << ATE_BGM_TUNE_R_OFFSET)) >> ATE_BGM_TUNE_R_OFFSET);
}

static inline unsigned char chip_bgm_buck_ana_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_ANA_MASK << ATE_BGM_ANA_OFFSET)) >> ATE_BGM_ANA_OFFSET);
}

static inline unsigned char chip_bgm_buck_dig_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_DIG_MASK << ATE_BGM_DIG_OFFSET)) >> ATE_BGM_DIG_OFFSET);
}

static inline unsigned char chip_bgm_op8_trim_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_0P8_TRIM_MASK << ATE_BGM_0P8_TRIM_OFFSET)) >> ATE_BGM_0P8_TRIM_OFFSET);
}

static inline unsigned char chip_bgm_op6_trim_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_0P6_TRIM_MASK << ATE_BGM_0P6_TRIM_OFFSET)) >> ATE_BGM_0P6_TRIM_OFFSET);
}

static inline unsigned char chip_bgm_iztc_trim_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_IZTC_TRIM_MASK << ATE_BGM_IZTC_TRIM_OFFSET)) >> ATE_BGM_IZTC_TRIM_OFFSET);
}

static inline unsigned char chip_bgm_ipta_trim_cal(void)
{
    return ((ate_efuse_info.data[2] & ((unsigned int)ATE_BGM_IPTATCAL_MASK << ATE_BGM_IPTATCAL_OFFSET)) >> ATE_BGM_IPTATCAL_OFFSET);
}

static inline unsigned char chip_lpldo_vtrim_func_cal(void)
{
    return ((ate_efuse_info.data[3] & ((unsigned int)ATE_LPLDO_VTRIM_FUNC_MASK << ATE_LPLDO_VTRIM_FUNC_OFFSET)) >> ATE_LPLDO_VTRIM_FUNC_OFFSET);
}

static inline unsigned char chip_lpldo_vtrim_sleep_cal(void)
{
    return ((ate_efuse_info.data[3] & ((unsigned int)ATE_LPLDO_VTRIM_SLEEP_MASK << ATE_LPLDO_VTRIM_SLEEP_OFFSET)) >> ATE_LPLDO_VTRIM_SLEEP_OFFSET);
}

static inline unsigned int chip_ate_chip_type(void)
{
    return ((ate_efuse_info.data[4] & ((unsigned int)ATE_CHIP_TYPE_MASK << ATE_CHIP_TYPE_OFFSET)) >> ATE_CHIP_TYPE_OFFSET);
}

static inline unsigned int chip_ate_ft_ver(void)
{
    return ((ate_efuse_info.data[4] & ((unsigned int)ATE_FT_VER_MASK << ATE_FT_VER_OFFSET)) >> ATE_FT_VER_OFFSET);
}

static inline unsigned int chip_ate_irr_trim(void)
{
    return ((ate_efuse_info.data[5] & ((unsigned int)ATE_IRR_TRIM_MASK << ATE_IRR_TRIM_OFFSET)) >> ATE_IRR_TRIM_OFFSET);
}
#endif

