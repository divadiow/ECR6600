/**
*@file	   i2s.c
*@brief    This part of program is I2S algorithm Software interface
*@author   weifeiting
*@data     2021.2.1
*@version  v0.1
*@par Copyright (c):
     eswin inc.
*@par History:
   version:autor,data,desc\n
*/

#define REG_PL_WR(addr, value)       (*(volatile unsigned int  *)(addr)) = (value)
#define REG_PL_RD(addr)              (*(volatile unsigned int *)(addr))
#define NULL ((void *)0)

/**
 * @brief I2S mode set
 */
typedef enum {
    I2S_MODE_MASTER = 0,
    I2S_MODE_SLAVE 
} i2s_mode_t;

/**
 * @brief I2S transfer data align format
 */	
typedef enum {
    DATA_ALIGN_LEFT = 0,
    DATA_ALIGN_RIGHT=1,
    DATA_ALIGN_I2S
} i2s_data_align_format_t;

/**
 * @brief I2S audio file format
 */
typedef enum {
    STEREO_MODE = 0,
    COMPRESSED_STEREO_MODE=1,
    MONO_MODE=2,
	RESERVE=3
} i2s_trans_mode;

/**
 * @brief I2S transfer data width
 */	
typedef enum {
	I2S_BIT_8_WIDTH = 1,
	I2S_BIT_16_WIDTH,
	I2S_BIT_32_WIDTH
} i2s_bit_width;

/**
 * @brief I2S compressed stereo data width
 */		
typedef enum {
	COMPRE_BIT_8_WIDTH = 0,
	COMPRE_BIT_16_WIDTH
} i2s_compre_bit_width;

/**
 * @brief I2S Initialize variable definition
 */
typedef struct {
    i2s_mode_t             mode;                  
    float           sample_rate;           
    i2s_bit_width   		   sample_width;             
    i2s_data_align_format_t       data_align_format;   
    i2s_trans_mode              trans_mode; 
	i2s_compre_bit_width        compre_bit_width; 
	unsigned int           i2s_dma;
} i2s_cfg_dev;

i2s_cfg_dev i2s_cfg;

/**
 * @brief I2S Initialize function
 */
int i2s_init_cfg(i2s_cfg_dev *i2s_cfg_dev);

/**
@brief Statement i2s close
*/
int drv_i2s_close();

/**
@brief     Statement i2s master transmit data
@param[in]  *bufptr : Buffer for sending data
@param[in]  length : The length of the data sent by the host
*/
void i2s_master_write(unsigned char * bufptr, unsigned int length);

/**
@brief     Statement i2c master receive data
@param[in]  *bufptr : The data read by the master is stored in this data
@param[in]  length : The length of the data the host wants to read
*/
void i2s_master_read(unsigned   char *bufptr, unsigned int length);
void i2s_master_read_loop(void);

