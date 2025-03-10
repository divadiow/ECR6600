/**
*@file	   i2c.h
*@brief    This part of program is I2C function declaration and variable definition
*@author   wei feiting
*@data     2021.1.29
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/
#include <stdint.h>
#define I2C_Address_7bit              0x00
#define I2C_Address_10bit             0x02
#define I2C_Mode_Slave                0x00 
#define I2C_Mode_Master               0x04

#define I2C_DMA_Disable               0x00
#define I2C_DMA_Enable                0x08
 
#define I2C_Speed_100K     				100000
#define I2C_Speed_400K     				400000
#define I2C_Speed_1M     				1024000
#define I2C_Disable                   0x00
#define I2C_Enable                    0x01

//#ifndef BIT
//#define BIT(x) (1 << (x))
//#endif

/**
 * @brief I2C Initialize variable definition.
 * @details The detail description.
 */
typedef struct
{
  unsigned int  I2C_Speed;
  unsigned char  I2C_AddressBit;
  unsigned char  I2C_Mode;
  unsigned int  I2C_DMA;
  unsigned char scl_num;
  unsigned char sda_num;
}I2C_InitTypeDef;


I2C_InitTypeDef i2c_master;

#define REG_PL_WR(addr, value)       (*(volatile unsigned int *)(addr)) = (value)
#define REG_PL_RD(addr)              (*(volatile unsigned int *)(addr))
#define NULL ((void *)0)

/**
@brief     Statement i2c interrupt 
*/
void i2c_isr(void);

/**
@brief     Statement i2c initialization 
*/
int i2c_init_cfg(I2C_InitTypeDef* I2C_InitStruct);

/**
@brief Statement i2c close
*/
int drv_i2c_close();

/**
@brief     Statement i2c master receive data
@param[in]  *slave_addr :slave device address
@param[in]  *bufptr : The data read by the master is stored in this data
@param[in]  len : The length of the data the host wants to read
@param[in]  cmd_len :Command length of slave
*/
int i2c_master_read(unsigned int slave_addr,unsigned char *bufptr, unsigned int len,unsigned int cmd_len);

/**
@brief     Statement i2c master transmit data
@param[in]  *slave_addr :slave device address
@param[in]  *bufptr : Buffer for sending data
@param[in]  len : The length of the data sent by the host
*/
int i2c_master_write(unsigned int slave_addr,unsigned char *bufptr, unsigned int len);

/**This variable is used to distinguish tx and rx of dma*/
extern unsigned int i2c_dma_flag ;
