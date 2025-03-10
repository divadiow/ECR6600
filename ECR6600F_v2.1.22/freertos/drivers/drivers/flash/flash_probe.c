
#include "flash.h"

#include "flash_internal.h"



#define FLASH_MANUFACTURER_ID(X)		((unsigned char)((X) & 0xFF))

typedef struct _T_FLASH_TABLE
{
	const unsigned char manufacturer_id;
	int (*probe)(T_SPI_FLASH_DEV *p_flash_dev);
} T_FLASH_TABLE;



static const T_FLASH_TABLE flash_tables [] = 
{
#ifdef CONFIG_FLASH_TH
		{FLASH_ID_TH, spiFlash_TH_probe},
#endif

#ifdef CONFIG_FLASH_GD
	{FLASH_ID_GD, spiFlash_GD_probe},
#endif

#ifdef CONFIG_FLASH_PUYA
	{FLASH_ID_PUYA, spiFlash_PUYA_probe},
#endif

#ifdef CONFIG_FLASH_FM
	{FLASH_ID_FM, spiFlash_FM_probe},
#endif

#ifdef CONFIG_FLASH_HUABANG
	{FLASH_ID_W, spiFlash_HB_probe},
#endif

#ifdef CONFIG_FLASH_XM
	{FLASH_ID_XM, spiFlash_XM_probe},
#endif

#ifdef CONFIG_FLASH_XT
	{FLASH_ID_XT, spiFlash_XT_probe},
#endif

#ifdef CONFIG_FLASH_ZB
	{FLASH_ID_ZB, spiFlash_ZB_probe},
#endif

#ifdef CONFIG_FLASH_EN
	{FLASH_ID_EN, spiFlash_EN_probe},
#endif

#ifdef CONFIG_FLASH_BOYA
	{FLASH_ID_BOYA, spiFlash_BOYA_probe},
#endif


};



int spiFlash_probe(T_SPI_FLASH_DEV *p_flash_dev)
{
	int i;
	
	for (i=0; i<FLASH_ARRAY_SIZE(flash_tables); i++)
	{
		if (flash_tables[i].manufacturer_id == FLASH_MANUFACTURER_ID(p_flash_dev->flash_param.flash_id))
		{
			if (flash_tables[i].probe(p_flash_dev) == 0)
			{
				return 0;
			}
		}
	}

	return -1;
}


