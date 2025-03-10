

#ifndef DRV_EFUSE_H
#define DRV_EFUSE_H


//#ifndef inw
//#define inw(reg)        (*((volatile unsigned int *) (reg)))
//#endif
//
//#ifndef outw
//#define outw(reg, data) ((*((volatile unsigned int *)(reg)))=(unsigned int)(data))
//#endif

#define BUFF_MAP_MASK 			0xfffffe00
#define SOFTWARE_RESET_REG		0x00202014
#define EFUSE_CLOSE				0x7fffffff
#define EFUSE_OPEN				0x80000000
#define EFUSE_AON_DATA			0x0070a7BC
#define AGCFSMRESET				0x0070b390

#define EFUSE_AON_LENGTH		36		//without include 4byte map
#define EFUSE_AON_MAX_INDEX		9
#define EFUSE_BUFF_LENGTH		128
#define EFUSE_BUFF_MAX_INDEX	32

//ate cal info
#define EFUSE_ATE_CAL_INFO_OFF  0x0
#define EFUSE_CHIP_ID           0x10

//efuse chip id type
#define EFUSE_CHIP_OFFSET         			27
#define EFUSE_CHIP_MASK         		  0x1f
#define EFUSE_CHIP_DCDC					0x1
#define EFUSE_CHIP_LDO					0x0



//MAC
#define EFUSE_MAC_ADDR 			0x18
#define EFUSE_MAC_FLAG_ADDR		0x1E

#define EFUSE_MAX_ADDR			0x7C
#define EFUSE_MAX_LENGTH		32

//efuse ctrl
#define EFUSE_CTRL_OFFSET                    		0x20
#define EFUSE_FLASH_ENCRYPTION_ASE_KEY_OFFSET		0x24
#define EFUSE_ECC_KEY_ADDR_OFFSET					0x34

#define EFUSE_JTAG_CTRL_EN							0x0
#define EFUSE_JTAG_CTRL_AUTO                    	0x1
#define EFUSE_JTAG_CTRL_DIS                    		0x3

#define EFUSE_SECURE_BOOT_EN                      	(0x1 << 0x2)
#define EFUSE_CTRL_FLASH_ENCRYPT_EN                	(0x1 << 0x3)
#define EFUSE_UART_AHB_READ_FLASH_DIS              	(0x1 << 0x4)
#define EFUSE_DIG_TEST_SHORT_DVDD_SW_EN            	(0x1 << 0x4)
#define EFUSE_CTRL_AES_KEY_READ_DIS                	(0x1 << 0x5)
#define EFUSE_CTRL_AES_KEY_WRITE_DIS              	(0x1 << 0x6)
#define EFUSE_CRYSTAL_40M                          	(0x1 << 16)
#define EFUSE_CRYSTAL_26M                          	(0x0 << 16)
#define EFUSE_LOG_PRINTF                           	(0x0 << 17)
#define EFUSE_LOG_MEMORY                          	(0x1 << 17)
#define EFUSE_SEL_160M                              (0x1 << 18)

#define EFUSE_CTRL_FLASH_ENCRYPT_CNT_MASK			(0x0000FF00)




#define EFUSE_RET_SUCCESS			0
#define EFUSE_RET_ERROR				-1
#define EFUSE_RET_PARAMETER_ERROR	-2
#define EFUSE_RET_POINT_NULL		-4

int drv_efuse_read(unsigned int addr,unsigned int * value, unsigned int length);
int drv_efuse_write(unsigned int addr, unsigned int value, unsigned int mask);
void drv_efuse_init(void);


#if 0
int drv_efuse_write_mac(unsigned char * mac);
int drv_efuse_read_mac(unsigned char * mac);
#endif


#endif /* DRV_EFUSE_H */

