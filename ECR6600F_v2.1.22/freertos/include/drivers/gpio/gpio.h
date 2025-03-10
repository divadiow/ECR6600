

#ifndef DRV_GPIO_H
#define DRV_GPIO_H

#include "chip_configuration.h"




/*  GPIO NUM */
#define GPIO_NUM_0			0x00
#define GPIO_NUM_1			0x01
#define GPIO_NUM_2			0x02
#define GPIO_NUM_3			0x03
#define GPIO_NUM_4			0x04
#define GPIO_NUM_5			0x05
#define GPIO_NUM_6			0x06
#define GPIO_NUM_7			0x07
#define GPIO_NUM_8			0x08
#define GPIO_NUM_9			0x09
#define GPIO_NUM_10		0x0A
#define GPIO_NUM_11		0x0B
#define GPIO_NUM_12		0x0C
#define GPIO_NUM_13		0x0D
#define GPIO_NUM_14		0x0E
#define GPIO_NUM_15		0x0F
#define GPIO_NUM_16		0x10
#define GPIO_NUM_17		0x11
#define GPIO_NUM_18		0x12
#define GPIO_NUM_19		0x13
#define GPIO_NUM_20		0x14
#define GPIO_NUM_21		0x15
#define GPIO_NUM_22		0x16
#define GPIO_NUM_23		0x17
#define GPIO_NUM_24		0x18
#define GPIO_NUM_25		0x19
#define GPIO_NUM_MAX  GPIO_NUM_25

#define GPIO_BASE			0x00201300

#define GPIO_ID_VER			GPIO_BASE
#define GPIO_DATA_IN		(GPIO_BASE + 0x20)
#define GPIO_DATA_OUT		(GPIO_BASE + 0x24)
#define GPIO_DIRECTION		(GPIO_BASE + 0x28)
#define GPIO_OUT_CLEAR		(GPIO_BASE + 0x2C)
#define GPIO_OUT_SET		(GPIO_BASE + 0x30)
#define GPIO_PULL_EN		(GPIO_BASE + 0x40)
#define GPIO_PULL_TYPE		(GPIO_BASE + 0x44)
#define GPIO_INT_EN			(GPIO_BASE + 0x50)
#define GPIO_INT_MODE(X)	(GPIO_BASE + 0x54 + 4 * (X))
#define GPIO_INT_STS		(GPIO_BASE + 0x64)


#define GPIO_ID_VAL				0x02031000
#define GPIO_ID_MASK			0xFFFFFF00



#define GPIO_RET_SUCCESS			0
#define GPIO_RET_ERROR				-1
#define GPIO_RET_PARAMETER_ERROR	-2
#define GPIO_RET_INIT_ERROR			-3
#define GPIO_RET_POINT_NULL			-4

#ifndef NULL
#define NULL ((void *)0)
#endif


typedef enum
{
	LEVEL_LOW	  = 0x00,
	LEVEL_HIGH	= 0x01,
    LEVEL_MAX	  = 0x02,
} GPIO_LEVEL_VALUE;


enum GPIO_DIR{
    GPIO_INPUT,
    GPIO_OUTPUT,
    GPIO_DIR_MAX
};
typedef struct _T_GPIO_ISR_CALLBACK
{
	void (* gpio_callback)(void *);
	void *gpio_data;
}T_GPIO_ISR_CALLBACK;


/*
  *   GPIO INTERRUPT MODE ARGUMENT    *
  */
#define DRV_GPIO_ARG_INTR_MODE_NOP			0
#define DRV_GPIO_ARG_INTR_MODE_HIGH			2
#define DRV_GPIO_ARG_INTR_MODE_LOW			3
#define DRV_GPIO_ARG_INTR_MODE_N_EDGE		5
#define DRV_GPIO_ARG_INTR_MODE_P_EDGE		6	
#define DRV_GPIO_ARG_INTR_MODE_DUAL_EDGE	7

/* 
  *   GPIO DIRCTION  ARGUMENT    *
  */
#define DRV_GPIO_ARG_DIR_IN					0
#define DRV_GPIO_ARG_DIR_OUT				1

/* 
  *   GPIO PULL TYPE  ARGUMENT    *
  */
#define DRV_GPIO_ARG_PULL_UP				0
#define DRV_GPIO_ARG_PULL_DOWN			1
#define DRV_GPIO_PULL_TYPE_MAX      2

#define DRV_GPIO_CTRL_REGISTER_ISR		0
#define DRV_GPIO_CTRL_INTR_MODE			1
#define DRV_GPIO_CTRL_INTR_ENABLE		2
#define DRV_GPIO_CTRL_INTR_DISABLE		3
#define DRV_GPIO_CTRL_SET_DIRCTION		4
#define DRV_GPIO_CTRL_PULL_TYPE			5
#define DRV_GPIO_CTRL_SET_DEBOUNCE		6
#define DRV_GPIO_CTRL_CLOSE_DEBOUNCE	7



#define AON_PULLDOWN_GPIO_REG 		0x20121c
#define AON_PULLUP_GPIO_REG			0x201218
#define FRQ_MHZ  					1000000L
#define TIME_NS  					1000L
#define EFUSE_FT_ADDR				0x10
#define EFUSE_FT_LENG				0x4
#define EFUSE_FT_MASK				0x04000000
#define EFUSE_FT_TYPE_1600			0x04000000
#define EFUSE_FT_TYPE_6600			0x00000000


#define DRV_GPIO_LEVEL_LOW			0
#define DRV_GPIO_LEVEL_HIGH			1

int drv_gpio_init(void);
int drv_gpio_read(int gpio_num);
int drv_gpio_write(int gpio_num, GPIO_LEVEL_VALUE gpio_level);
int drv_gpio_dir_get(int gpio_num);
int drv_gpio_ioctrl(int gpio_num, int event, int arg);
int drv_gpio_pull_read(int gpio_num);
extern unsigned int efuse_ft;

#endif /* DRV_GPIO_H */

