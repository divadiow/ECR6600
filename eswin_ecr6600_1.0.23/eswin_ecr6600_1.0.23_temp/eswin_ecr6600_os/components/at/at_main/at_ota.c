#include "at_ota.h"
#include "easyflash.h"
#include "flash.h"
#include "http_ota.h"
#include "ota.h"
#include "hal_system.h"

#ifdef CONFIG_OTA

static dce_result_t dce_handle_CIUPDATE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if(((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_STRING) /*||
            ((kind & DCE_WRITE) && argc == 2 && 
            argv[0].type == ARG_TYPE_STRING && 
            argv[1].type == ARG_TYPE_NUMBER && 
            (argv[1].value.number == 1 || argv[1].value.number == 0))*/)
    {
        if (SYS_OK == http_client_download_file(argv[0].value.string, strlen(argv[0].value.string)))
        {
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            ota_done(1);
            return DCE_OK;
        }
    }

    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    return DCE_INVALID_INPUT;
}

static dce_result_t dce_handle_CICHANGE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    image_headinfo_t imagehead = {0};
    unsigned char image_active_part;
    ota_state_t otastate = {0};
    unsigned int addr;
    unsigned int len;
    unsigned int calcrc;

    if(((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_NUMBER))
    {
        if (argv[0].value.number == 0)
        {
            dce_emit_basic_result_code(dce, DCE_RC_OK); 
            hal_system_reset(RST_TYPE_OTA);
            return DCE_OK;
        }

        if (partion_info_get(PARTION_NAME_CPU, &addr, &len) != 0)
        {
            os_printf(LM_APP, LL_ERR, "cichange can not get %s info\n", PARTION_NAME_CPU);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        // get active part
        extern void *__etext1;
        unsigned int text_addr = ((unsigned int)&__etext1) & 0x1FFFFF;
        os_printf(LM_APP, LL_ERR, "etext1:0x%x\n", text_addr);
        if (text_addr < addr + len / 2)
        {
            addr = addr + len / 2;
            image_active_part = OTA_UPDATE_ACTIVE_PARTA;
        }
        else
        {
            image_active_part = OTA_UPDATE_ACTIVE_PARTB;
        }

        memset(&imagehead, 0, sizeof(imagehead));
        drv_spiflash_read(addr, (unsigned char *)&imagehead, sizeof(imagehead));
        if (imagehead.update_method != OTA_UPDATE_METHOD_AB)
        {
            os_printf(LM_APP, LL_ERR, "cichange not support method 0x%x\n", imagehead.update_method);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        calcrc = ota_get_flash_crc(addr, offsetof(image_headinfo_t, image_hcrc));
        if (calcrc != imagehead.image_hcrc)
        {
            os_printf(LM_APP, LL_ERR, "cichange calcrc 0x%x hcrc 0x%x\n", calcrc, imagehead.image_hcrc);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        if (partion_info_get(PARTION_NAME_OTA_STATUS, &addr, &len) != 0)
        {
            os_printf(LM_APP, LL_ERR, "cichange can not get %s info\n", PARTION_NAME_OTA_STATUS);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }
        drv_spiflash_read(addr,(unsigned char *)&otastate, sizeof(otastate));

        if (imagehead.image_dcrc != 0)
        {
            calcrc = ota_get_flash_crc(addr, imagehead.data_size + imagehead.xip_size + imagehead.text_size + sizeof(imagehead));
            if (calcrc != imagehead.image_dcrc)
            {
                os_printf(LM_APP, LL_ERR, "cichange calcrc 0x%x dcrc 0x%x\n", calcrc, imagehead.image_dcrc);
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_FAILED;
            }
        }
        os_printf(LM_APP, LL_ERR, "imageselect.active_part:%x \n",image_active_part);

        if(image_active_part == OTA_UPDATE_ACTIVE_PARTB)
        {
            otastate.updata_ab = OTA_AB_UPDATAFLAG;
        }
        else
        { 
            otastate.updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B;
        }
        otastate.update_flag = 1;
        int crclen = offsetof(ota_state_t, patch_addr);
        otastate.crc = ef_calc_crc32(0, (char *)&otastate + crclen, sizeof(otastate) - crclen);
        
        len = len / 2;
        drv_spiflash_erase(addr, len);
        drv_spiflash_write(addr, (unsigned char *)&otastate, sizeof(otastate));
        drv_spiflash_erase(addr + len, len);
        drv_spiflash_write(addr + len, (unsigned char *)&otastate, sizeof(otastate));
        dce_emit_basic_result_code(dce, DCE_RC_OK);
        hal_system_reset(RST_TYPE_OTA);
        return DCE_OK;
    }

    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    return DCE_FAILED;
}

static const command_desc_t UPDATA_commands[] = {
    {"CIUPDATE"   , &dce_handle_CIUPDATE,    DCE_TEST| DCE_WRITE | DCE_READ},
    {"CICHANGE"   , &dce_handle_CICHANGE,    DCE_TEST| DCE_WRITE | DCE_READ},
};
#endif

void dce_register_ota_commands(dce_t* dce)
{
#ifdef CONFIG_OTA
    dce_register_command_group(dce, "OTA", UPDATA_commands, sizeof(UPDATA_commands) / sizeof(command_desc_t), 0);
#endif
}



