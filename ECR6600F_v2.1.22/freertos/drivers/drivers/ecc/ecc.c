
/**
* @file       uart.c
* @brief      ecc driver code  
* @author     Wangchao
* @date       2021-9-17
* @version    v0.1
* @par Copyright (c):
*      eswin inc.
* @par History:        
*   version: author, date, desc\n
*/

#include "chip_smupd.h"
#include "chip_configuration.h"
#include "arch_irq.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "chip_clk_ctrl.h"
#include "ecc.h"
#include "encrypt_lock.h"
#ifdef CONFIG_PSM_SURPORT
#include "psm_system.h"
#endif

typedef volatile struct _T_ECC_REG_MAP
{
	unsigned int ctrl;			// 0x00
	unsigned int conf;			// 0x04
	unsigned int resv0[2];		// 0x08 ~ 0x0c
	unsigned int mc_ptr;		// 0x10
	unsigned int resv1[3];		// 0x14 ~ 0x1c
	unsigned int stat;			// 0x20
	unsigned int rt_code;		// 0x24
	unsigned int resv2[10];		// 0x28 ~ 0x4c
	unsigned int exc_conf;		// 0x50
	unsigned int resv3[11];		// 0x54 ~ 0x7c
	unsigned int version;		// 0x80
	unsigned int resv4[223];	// 0x84 ~ 0x3fc

	//unsigned int mem_a[136];	// 0x400 ~ 0x61c

	unsigned int mem_a0[9];	// 0x400 ~ 0x420
	unsigned int mem_a1[9];	// 0x424 ~ 0x444
	unsigned int mem_a2[9];	// 0x448 ~ 0x468
	unsigned int mem_a3[9];	// 0x46c ~ 0x48c
	unsigned int mem_a4[9];	// 0x490 ~ 0x4b0
	unsigned int mem_a5[9];	// 0x4b4 ~ 0x4d0

	unsigned int resv5[714];	// 0x4d4 ~ 0xffc

	unsigned int mem_b0[9];	// 0x1000 ~ 0x1020
	unsigned int mem_b1[9];	// 0x1024 ~ 0x1044
	unsigned int mem_b2[9];	// 0x1048 ~ 0x1068
	unsigned int mem_b3[9];	// 0x106c ~ 0x108c
	unsigned int mem_b4[9];	// 0x1090 ~ 0x10b0

	//unsigned int mem_b[220];	// 0x1000 ~ 0x121c
} T_ECC_REG_MAP;

/* control */
#define ECC_CTRL_GO							1
#define ECC_CTRL_STOP						(1<<16)

/* configuration */
#define ECC_CONF_BASE_RADIX			(2<<24)
#define ECC_CONF_PARTIAL_RADIX(X)		(((X))<<16)
#define ECC_CONF_IRQEN					(1<<8)

/* microcode pointer */
#define ECC_MICROCODE_POINT_DBL				0x04
#define ECC_MICROCODE_POINT_ADD				0x08
#define ECC_MICROCODE_POINT_VER				0x0c
#define ECC_MICROCODE_POINT_MUL				0x10
#define ECC_MICROCODE_MOD_MUL				0x18
#define ECC_MICROCODE_MOD_INV					0x1c
#define ECC_MICROCODE_MOD_ADD				0x20
#define ECC_MICROCODE_MOD_SUB				0x24
#define ECC_MICROCODE_CAL_PRE_MON			0x28
#define ECC_MICROCODE_C25519_POINT_MUL		0x34
#define ECC_MICROCODE_ED25519_POINT_MUL		0x38
#define ECC_MICROCODE_ED25519_POINT_ADD		0x3c

/* status  */
#define ECC_STATUS_DONE				1

/* return code */
#define ECC_RETURN_CODE_NORMAL		0
#define ECC_RETURN_CODE_STOP		1

/* exccution configuration */
#define ECC_CFG_OMON					0x20
#define ECC_CFG_OAFF					0x10
#define ECC_CFG_IAFF_R1				0x04
#define ECC_CFG_IMON_R0				0x02
#define ECC_CFG_IAFF_R0				0x01



#define ECC_MAP_BASE	((T_ECC_REG_MAP *)MEM_BASE_ECC)

#define DRV_ECC_RAM_VALUE  1 
#define DRV_ECC_ENDIAN_VALUE_MSK  0xFFFFFFFE 


/* Curve selection options. */
#define ECC_SECP128R1	16
#define ECC_SECP192R1	24
#define ECC_SECP256R1	32
#define ECC_SECP384R1	48


/* Now support nist p256 only!!!!!!!!!!!!!!!!!. */
/* y^2 = x^3 -3 * x +2 mod p */
#define ECC_CURVE 	ECC_SECP256R1

#define ECC_BYTES 	ECC_CURVE
#define ECC_INTS 	(ECC_BYTES/(sizeof(unsigned int)))

#define ECC_RET_CHK(f) 	do { if( ( ret = (f) ) != 0 ) goto finish; } while( 0 )

#define DRV_USE_ROM_INIT_ECC	1

#if DRV_USE_ROM_INIT_ECC
#define DRV_ECC256_CURVE_A_ADDR	    	0x0000B438
#define DRV_ECC256_CURVE_MOD_ADDR	0x0000B418
#define DRV_ECC256_CURVE_B_ADDR	    	0x0000B3F8

unsigned int *ecc256_curve_a = (unsigned int *)DRV_ECC256_CURVE_A_ADDR;
unsigned int *ecc256_curve_mod = (unsigned int *)DRV_ECC256_CURVE_MOD_ADDR;
unsigned int *ecc256_curve_b = (unsigned int *)DRV_ECC256_CURVE_B_ADDR;

#else
static unsigned int  ecc256_curve_a[ECC_INTS] = 
{
	0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
};


static unsigned int  ecc256_curve_mod[ECC_INTS] = 
{
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
	0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF
};


unsigned int ecc256_curve_b[] = 
{	0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0, 
	0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8
};
#endif

static char drv_ecc_init;
static os_sem_handle_t drv_ecc_sem;
static void_fn drv_ecc_isr_callback;


static void drv_ecc_isr(void) __attribute__((section(".ilm_text_drv")));
void drv_ecc256_point_mult_abort(void) __attribute__((section(".ilm_text_drv")));


static void drv_ecc_isr(void)
{
	#ifdef CONFIG_SYSTEM_IRQ
	unsigned long flags = system_irq_save();
	#endif
	
	ECC_MAP_BASE->stat = 0;

	if (drv_ecc_isr_callback)
	{
		drv_ecc_isr_callback();
		drv_ecc_isr_callback = NULL;
	}
	else
	{
		os_sem_post(drv_ecc_sem);
	}

	#ifdef CONFIG_SYSTEM_IRQ
	system_irq_restore(flags);
	#endif
}


static void drv_ecc256_set_para_mon(void)
{
	ECC_MAP_BASE->mem_b4[0] =  0x00000001;

	ECC_MAP_BASE->mem_a3[0] =  0x00000003;
	ECC_MAP_BASE->mem_a3[1] =  0x00000000;
	ECC_MAP_BASE->mem_a3[2] =  0xFFFFFFFF;
	ECC_MAP_BASE->mem_a3[3] =  0xFFFFFFFB;
	ECC_MAP_BASE->mem_a3[4] =  0xFFFFFFFE;
	ECC_MAP_BASE->mem_a3[5] =  0xFFFFFFFF;
	ECC_MAP_BASE->mem_a3[6] =  0xFFFFFFFD;
	ECC_MAP_BASE->mem_a3[7] =  0x00000004;
}


static int drv_ecc256_cal_pre_mon(void)
{
	int i;

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_b3[i] = ecc256_curve_mod[i];
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_CAL_PRE_MON;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	if (ECC_MAP_BASE->rt_code)
	{
		return -ECC_MAP_BASE->rt_code;
	}

	return 0;
}


int drv_ecc256_mod_add(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_a0[i] = *(p_left + i);
		ECC_MAP_BASE->mem_b0[i] = *(p_right + i);
		ECC_MAP_BASE->mem_b3[i] = ecc256_curve_mod[i];
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_MOD_ADD;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*(p_result + i) = ECC_MAP_BASE->mem_a0[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


int drv_ecc256_mod_mult(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	ECC_RET_CHK(drv_ecc256_cal_pre_mon());

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_a0[i] = *(p_left + i);
		ECC_MAP_BASE->mem_b0[i] = *(p_right + i);
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_MOD_MUL;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OAFF | ECC_CFG_IAFF_R1 | ECC_CFG_IAFF_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*(p_result + i) = ECC_MAP_BASE->mem_a0[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}



int drv_ecc256_mod_sub(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_a0[i] = *(p_left + i);
		ECC_MAP_BASE->mem_b0[i] = *(p_right + i);
		ECC_MAP_BASE->mem_b3[i] = ecc256_curve_mod[i];
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_MOD_SUB;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*(p_result + i) = ECC_MAP_BASE->mem_a0[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


int drv_ecc256_mod_inv(unsigned int *p_result, unsigned int *p_input)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_b0[i] = *(p_input + i);
		ECC_MAP_BASE->mem_b3[i] = ecc256_curve_mod[i];
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_MOD_INV;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*( p_result + i) = ECC_MAP_BASE->mem_a0[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}




int drv_ecc256_point_mult(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_x, unsigned int *p_point_y, 
							unsigned int *p_scalar)
{
	int i, ret;

	drv_encrypt_lock();
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_ECC,PSM_DEVICE_STATUS_ACTIVE);
	#endif
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	if (!drv_ecc_init)
	{
		drv_ecc_sem = os_sem_create(1, 0);
		ECC_MAP_BASE->stat = 0;
		arch_irq_register(VECTOR_NUM_ECC, drv_ecc_isr);
		arch_irq_clean(VECTOR_NUM_ECC);
		arch_irq_unmask(VECTOR_NUM_ECC);
		drv_ecc_init = 1;
	}

	drv_ecc256_set_para_mon();

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_b0[i] = *(p_point_x + i);
		ECC_MAP_BASE->mem_b1[i] = *(p_point_y + i);
		ECC_MAP_BASE->mem_a5[i] = *(ecc256_curve_a + i);
		ECC_MAP_BASE->mem_a4[i] = *(p_scalar + i);
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_POINT_MUL;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OAFF | ECC_CFG_IAFF_R1 | ECC_CFG_IAFF_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS) |ECC_CONF_IRQEN;
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	os_sem_wait(drv_ecc_sem, WAIT_FOREVER);

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*( p_result_x + i) = ECC_MAP_BASE->mem_a0[i];
		*( p_result_y + i) = ECC_MAP_BASE->mem_a1[i];
	}

finish:
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_ECC,PSM_DEVICE_STATUS_IDLE);
	#endif
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


int drv_ecc256_point_dbl(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_x, unsigned int *p_point_y)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	drv_ecc256_set_para_mon();

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_a0[i] = *(p_point_x+ i);
		ECC_MAP_BASE->mem_a1[i] = *(p_point_y + i);
		ECC_MAP_BASE->mem_a5[i] = *(ecc256_curve_a + i);
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_POINT_DBL;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OAFF | ECC_CFG_IAFF_R1 | ECC_CFG_IAFF_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*( p_result_x + i) = ECC_MAP_BASE->mem_a0[i];
		*( p_result_y + i) = ECC_MAP_BASE->mem_a1[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}



int drv_ecc256_point_add(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_left_x, unsigned int *p_point_left_y, 
							unsigned int *p_point_right_x, unsigned int *p_point_right_y)
{
	int i, ret;

	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	drv_ecc256_set_para_mon();

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_a0[i] = *( p_point_left_x + i);
		ECC_MAP_BASE->mem_a1[i] = *( p_point_left_y + i);
		ECC_MAP_BASE->mem_b0[i] = *( p_point_right_x + i);
		ECC_MAP_BASE->mem_b1[i] = *( p_point_right_y + i);
		ECC_MAP_BASE->mem_a5[i] = *(ecc256_curve_a + i);
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_POINT_ADD;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OAFF | ECC_CFG_IAFF_R1 | ECC_CFG_IAFF_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*( p_result_x + i) = ECC_MAP_BASE->mem_a0[i];
		*( p_result_y + i) = ECC_MAP_BASE->mem_a1[i];
	}

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


int drv_ecc256_point_verify(unsigned int *p_point_x, unsigned int *p_point_y)
{
	int i, ret;
	
	drv_encrypt_lock();
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	drv_ecc256_set_para_mon();

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_b0[i] = *( p_point_x + i);
		ECC_MAP_BASE->mem_b1[i] = *( p_point_y + i);
		ECC_MAP_BASE->mem_a5[i] = ecc256_curve_a[i];
		ECC_MAP_BASE->mem_b3[i] = ecc256_curve_mod[i];
		ECC_MAP_BASE->mem_a4[i] = ecc256_curve_b[i];
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_POINT_VER;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OMON | ECC_CFG_IAFF_R1 | ECC_CFG_IMON_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS);
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;

	while ( ECC_MAP_BASE->stat != ECC_STATUS_DONE);

	ECC_MAP_BASE->stat = 0;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

finish:
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


void drv_ecc256_point_mult_begin(unsigned int *p_point_x, unsigned int *p_point_y, 
							unsigned int *p_scalar, void_ecc_fn fn)
{
	int i;

	drv_encrypt_lock();
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_ECC,PSM_DEVICE_STATUS_ACTIVE);
	#endif
	chip_clk_enable((unsigned int)CLK_ECC);
	WRITE_REG(CHIP_SMU_PD_ENDIAN, READ_REG(CHIP_SMU_PD_ENDIAN) & DRV_ECC_ENDIAN_VALUE_MSK);
	WRITE_REG(CHIP_SMU_PD_RAM_ECC_HASH_SEL, DRV_ECC_RAM_VALUE);

	if (!drv_ecc_init)
	{
		drv_ecc_sem = os_sem_create(1, 0);
		ECC_MAP_BASE->stat = 0;
		arch_irq_register(VECTOR_NUM_ECC, drv_ecc_isr);
		arch_irq_clean(VECTOR_NUM_ECC);
		arch_irq_unmask(VECTOR_NUM_ECC);
		drv_ecc_init = 1;
	}

	drv_ecc_isr_callback = fn;

	drv_ecc256_set_para_mon();

	for (i = 0; i < ECC_INTS; i++)
	{
		ECC_MAP_BASE->mem_b0[i] = *(p_point_x + i);
		ECC_MAP_BASE->mem_b1[i] = *(p_point_y + i);
		ECC_MAP_BASE->mem_a5[i] = *(ecc256_curve_a + i);
		ECC_MAP_BASE->mem_a4[i] = *(p_scalar + i);
	}

	ECC_MAP_BASE->mc_ptr = ECC_MICROCODE_POINT_MUL;
	ECC_MAP_BASE->exc_conf = ECC_CFG_OAFF | ECC_CFG_IAFF_R1 | ECC_CFG_IAFF_R0;
	ECC_MAP_BASE->conf = ECC_CONF_BASE_RADIX |ECC_CONF_PARTIAL_RADIX(ECC_INTS) |ECC_CONF_IRQEN;
	ECC_MAP_BASE->ctrl = ECC_CTRL_GO;
}


int drv_ecc256_point_mult_result(unsigned int *p_result_x, unsigned int *p_result_y)
{
	int i, ret;

	ECC_RET_CHK(-ECC_MAP_BASE->rt_code);

	for (i = 0; i < ECC_INTS; i++)
	{
		*( p_result_x + i) = ECC_MAP_BASE->mem_a0[i];
		*( p_result_y + i) = ECC_MAP_BASE->mem_a1[i];
	}

finish:
	#ifdef CONFIG_PSM_SURPORT
	psm_set_device_status(PSM_DEVICE_ECC,PSM_DEVICE_STATUS_IDLE);
	#endif
	chip_clk_disable((unsigned int)CLK_ECC);
	drv_encrypt_unlock();
	return ret;
}


void drv_ecc256_point_mult_abort(void)
{
	unsigned long  flags = system_irq_save();
	if (drv_ecc_isr_callback)
	{
		ECC_MAP_BASE->ctrl = ECC_CTRL_STOP;		// ECC stops after one clock cycle
		__nds32__nop();
//		__nds32__nop();
		drv_ecc_isr_callback = NULL;				// Also used for delay
		ECC_MAP_BASE->stat = 0;
		arch_irq_clean(VECTOR_NUM_ECC);
		system_irq_restore(flags);
		#ifdef CONFIG_PSM_SURPORT
		psm_set_device_status(PSM_DEVICE_ECC,PSM_DEVICE_STATUS_IDLE);
		#endif
		chip_clk_disable((unsigned int)CLK_ECC);
		drv_encrypt_unlock();	
	}
	else
	{
		ECC_MAP_BASE->ctrl = ECC_CTRL_STOP;		// ECC stops after one clock cycle
		system_irq_restore(flags);
	}
}

