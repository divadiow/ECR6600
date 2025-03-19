#include "chip_memmap.h"
#include "chip_configuration.h"
#include "reg_macro_def.h"
#include "efuse.h"
#include "cli.h"


ate_cal_info_t ate_efuse_info;

#define MOVE_AGCRAM_TO_CPU    WRITE_REG(0x70B390, READ_REG(0x70B390) | (1 << 12));//AGCFSMRESET 
#define MOVE_AGCRAM_TO_MODEM  WRITE_REG(0x70B390,  READ_REG(0x70B390) & 0xFFFFEFFF);//AGCFSMRESET switch to AGC FSM

#define ATE_IFNO_AGC_RAM_ADDR   0x70a7bc

void init_ate_efuse_info(void)
{
    memset(&ate_efuse_info, 0, sizeof(ate_cal_info_t));
    drv_efuse_read(EFUSE_ATE_CAL_INFO_OFF, (unsigned int *)&ate_efuse_info, sizeof(ate_cal_info_t));
#if 0
    if (READ_REG(MEM_BASE_PCU + 0x28) & 0x1) { //read from agc ram
        MOVE_AGCRAM_TO_CPU
        memcpy((void *)&ate_efuse_info, (void *)ATE_IFNO_AGC_RAM_ADDR, sizeof(ate_cal_info_t));
        MOVE_AGCRAM_TO_MODEM

    } else { //read from efuse
        drv_efuse_read(0x0, (unsigned int *)&ate_efuse_info, sizeof(ate_cal_info_t));
        MOVE_AGCRAM_TO_CPU
        memcpy((void *)ATE_IFNO_AGC_RAM_ADDR, (void *)&ate_efuse_info, sizeof(ate_cal_info_t));
        MOVE_AGCRAM_TO_MODEM
    }
#endif
    //validate the efuse info
    //validate_efuse_info();
    
    return;
}

void dump_ate_efuse_info(void)
{
    os_printf(LM_OS, LL_INFO, "ate efuse info-->\n0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n<--\n", ate_efuse_info.data[0],
        ate_efuse_info.data[1], ate_efuse_info.data[2], ate_efuse_info.data[3], ate_efuse_info.data[4],
        ate_efuse_info.data[5], ate_efuse_info.data[6], ate_efuse_info.data[7]);
}

