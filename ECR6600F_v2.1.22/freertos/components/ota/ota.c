#include <stddef.h>
#include <string.h>
#include "flash.h"
#include "easyflash.h"
#include "oshal.h"
#include "ota.h"
#include "hal_system.h"

typedef enum
{
    DOWNLOAD_IDLE_ST,
    DOWNLOAD_HEAD_ST,
    DOWNLOAD_BOOT_ST,
    DOWNLOAD_IMGE_ST,
    DOWNLOAD_IMGA_ST,
    DOWNLOAD_IMGB_ST,
    DOWNLOAD_FULL_ST,
} download_state_e;

typedef enum
{
    DOWNLOAD_COMZ_METHOD,
    DOWNLOAD_DIFF_METHOD,
    DOWNLOAD_DUAL_METHOD,
} download_method_e;

typedef enum
{
    DOWNLOAD_BOOT_PART,
    DOWNLOAD_IMGA_PART,
    DOWNLOAD_IMGB_PART,
    DOWNLOAD_OTAS_PART,
    DOWNLOAD_FULL_PART,
} download_part_e;

typedef enum
{
    DOWNLOAD_NONE_ERR,
    DOWNLOAD_METH_ERR,
    DOWNLOAD_INIT_ERR,
    DOWNLOAD_INITD_ERR,
    DOWNLOAD_NOMEM_ERR,
    DOWNLOAD_PART_ERR,
    DOWNLOAD_MAGIC_ERR,
    DOWNLOAD_SIZE_ERR,
    DOWNLOAD_CRC_ERR,
    DOWNLOAD_STAT_ERR,
} download_err_e;

typedef struct
{
    ota_package_head_t dw_head;
    download_state_e dw_state;
    download_method_e dw_lmethod;
    download_method_e dw_rmethod;
    int dw_status;
    unsigned char *dw_buff;
    unsigned int dw_buff_offt;
    unsigned int dw_buff_size;
    unsigned int dw_etext;
    unsigned int dw_skip;
    unsigned int dw_addr;
    unsigned int dw_offt;
    unsigned int dw_dlen;
    unsigned int dw_percent;
    unsigned char dw_active_part;
    unsigned int dw_paddr[DOWNLOAD_FULL_PART];
    unsigned int dw_pdlen[DOWNLOAD_FULL_PART];
} download_oper_t;

download_oper_t *g_ota_download;
static void download_mem_release(download_oper_t **handle);
static int download_packet_data(download_oper_t *handle, unsigned char *data, unsigned int len);

#define DOWNLOAD_RET_CHECK_RETURN(ret) \
    {\
        if ((ret) != 0) \
        {\
            os_printf(LM_APP, LL_ERR, "%s[%d]ret:%d\n", __FUNCTION__, __LINE__, ret); \
            if (g_ota_download) g_ota_download->dw_status = ret; \
            download_mem_release(&g_ota_download); \
            return (-DOWNLOAD_INIT_ERR);\
        }\
    }

static void download_packet_head_dump(download_oper_t *handle)
{
    os_printf(LM_APP, LL_ERR, "magic:%c%c%c%c%c%c%c\n", handle->dw_head.magic[0], handle->dw_head.magic[1],
        handle->dw_head.magic[2], handle->dw_head.magic[3], handle->dw_head.magic[4],
        handle->dw_head.magic[5], handle->dw_head.magic[6]);
    os_printf(LM_APP, LL_ERR, "version:0x%x\n", handle->dw_head.version);
    os_printf(LM_APP, LL_ERR, "packsize:0x%x\n", handle->dw_head.package_size);
    os_printf(LM_APP, LL_ERR, "packcrc:0x%x\n", handle->dw_head.package_crc);
    os_printf(LM_APP, LL_ERR, "bootsize:0x%x\n", handle->dw_head.boot_size);
    os_printf(LM_APP, LL_ERR, "firmsize:0x%x\n", handle->dw_head.firmware_size);
    os_printf(LM_APP, LL_ERR, "firmnewsize:0x%x\n", handle->dw_head.firmware_new_size);
    os_printf(LM_APP, LL_ERR, "firmcrc:0x%x\n", handle->dw_head.firmware_crc);
    os_printf(LM_APP, LL_ERR, "firmnewcrc:0x%x\n", handle->dw_head.firmware_new_crc);
    os_printf(LM_APP, LL_ERR, "srcversion:%s\n", handle->dw_head.source_version);
    os_printf(LM_APP, LL_ERR, "dstversion:%s\n", handle->dw_head.target_version);
}

static void download_operate_dump(download_oper_t *handle)
{
    int i;
    os_printf(LM_APP, LL_ERR, "##############OTA DOWNLOAD##################\n");
    download_packet_head_dump(handle);
    os_printf(LM_APP, LL_ERR, "state:%d\n", handle->dw_state);
    os_printf(LM_APP, LL_ERR, "lmethod:%d\n", handle->dw_lmethod);
    os_printf(LM_APP, LL_ERR, "rmethod:%d\n", handle->dw_rmethod);
    os_printf(LM_APP, LL_ERR, "status:%d\n", handle->dw_status);
    os_printf(LM_APP, LL_ERR, "buffoft:%d\n", handle->dw_buff_offt);
    os_printf(LM_APP, LL_ERR, "buffsize:%d\n", handle->dw_buff_size);
    os_printf(LM_APP, LL_ERR, "dw_etext:0x%x\n", handle->dw_etext);
    os_printf(LM_APP, LL_ERR, "dw_skip:0x%x\n", handle->dw_skip);
    os_printf(LM_APP, LL_ERR, "dw_addr:0x%x\n", handle->dw_addr);
    os_printf(LM_APP, LL_ERR, "dw_offt:0x%x\n", handle->dw_offt);
    os_printf(LM_APP, LL_ERR, "dw_dlen:0x%x\n", handle->dw_dlen);
    os_printf(LM_APP, LL_ERR, "active:0x%x\n", handle->dw_active_part);
    for (i = 0; i < DOWNLOAD_FULL_PART; i++)
    {
        os_printf(LM_APP, LL_ERR, "paddr:0x%x\n", handle->dw_paddr[i]);
        os_printf(LM_APP, LL_ERR, "pdlen:0x%x\n", handle->dw_pdlen[i]);
    }
    os_printf(LM_APP, LL_ERR, "############################################\n");
}

static int download_get_lmethod(download_oper_t *handle)
{
    int ret;
    image_headinfo_t image_head;
    unsigned int start_addr = handle->dw_paddr[DOWNLOAD_IMGA_PART];

    if (handle->dw_active_part == DOWNLOAD_OTA_PARTB)
    {
        handle->dw_lmethod = DOWNLOAD_DUAL_METHOD;
    }
    else
    {
        ret = drv_spiflash_read(start_addr, (unsigned char *)&image_head, sizeof(image_headinfo_t));
        DOWNLOAD_RET_CHECK_RETURN(ret);
        handle->dw_lmethod = (image_head.update_method >> 4) / 4;
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_get_active_part(download_oper_t *handle)
{
    extern void *__etext1;
    /* __etext1 is XIP section start addr, 
    We can confirm whether partition A or B is running by judging the value of __etext1, 
    However, the value of etext1 is a complete flash address, 
    which contains a prefix of 0x408* ****, 
    but there is no prefix in the partition table, 
    so bit operation is required to remove it, 
    0x1FFFFF for 2M flash, not support 4M/8M flash, 
    0x7FFFFF support <=8M flash */
    unsigned int text_addr = ((unsigned int)&__etext1) & 0x7FFFFF;
    image_headinfo_t image_head;
    int ret;

    handle->dw_etext = text_addr;
    handle->dw_active_part = text_addr > handle->dw_paddr[DOWNLOAD_IMGB_PART] ? DOWNLOAD_OTA_PARTB : DOWNLOAD_OTA_PARTA;

    if (text_addr > handle->dw_paddr[DOWNLOAD_IMGB_PART])
    {
        ret = drv_spiflash_read(handle->dw_paddr[DOWNLOAD_IMGB_PART], (unsigned char *)&image_head, sizeof(image_headinfo_t));
        DOWNLOAD_RET_CHECK_RETURN(ret);

        if (ef_calc_crc32(0, &image_head, offsetof(image_headinfo_t, image_hcrc)) != image_head.image_hcrc)
        {
            handle->dw_active_part = DOWNLOAD_OTA_PARTA;
        }
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_partition_init(download_oper_t *handle)
{
    unsigned int get_addr;
    unsigned int get_len;
    int ret;

    handle->dw_status = partion_info_get(PARTION_NAME_BOOT, &get_addr, &get_len);
    DOWNLOAD_RET_CHECK_RETURN(handle->dw_status);
    handle->dw_paddr[DOWNLOAD_BOOT_PART] = get_addr;
    handle->dw_pdlen[DOWNLOAD_BOOT_PART] = get_len;

    ret = partion_info_get(PARTION_NAME_CPU, &get_addr, &get_len);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    handle->dw_paddr[DOWNLOAD_IMGA_PART] = get_addr;
    handle->dw_pdlen[DOWNLOAD_IMGA_PART] = get_len/2;
    handle->dw_paddr[DOWNLOAD_IMGB_PART] = get_addr + get_len/2;
    handle->dw_pdlen[DOWNLOAD_IMGB_PART] = get_len/2;

    ret = partion_info_get(PARTION_NAME_OTA_STATUS, &get_addr, &get_len);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    handle->dw_paddr[DOWNLOAD_OTAS_PART] = get_addr;
    handle->dw_pdlen[DOWNLOAD_OTAS_PART] = get_len;

    return DOWNLOAD_NONE_ERR;
}

static int download_packet_flash(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    unsigned int writelen;
    int ret;

    writelen = handle->dw_buff_size - handle->dw_buff_offt;
    if (writelen > len)
    {
        memcpy(handle->dw_buff + handle->dw_buff_offt, data, len);
        handle->dw_buff_offt += len;
    }
    else
    {
        memcpy(handle->dw_buff + handle->dw_buff_offt, data, writelen);
        ret = drv_spiflash_erase(handle->dw_addr + handle->dw_offt, handle->dw_buff_size);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        ret = drv_spiflash_write(handle->dw_addr + handle->dw_offt, handle->dw_buff, handle->dw_buff_size);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        handle->dw_buff_offt = 0;
        handle->dw_offt += handle->dw_buff_size;
        memset(handle->dw_buff, 0xFF, handle->dw_buff_size);
        if (len - writelen)
        {
            return download_packet_flash(handle, data + writelen, len - writelen);
        }
    }

    if (handle->dw_state > DOWNLOAD_BOOT_ST && handle->dw_skip == 0)
    {
        if (handle->dw_percent != (handle->dw_offt + handle->dw_buff_offt) * 100/handle->dw_dlen)
        {
            handle->dw_percent = (handle->dw_offt + handle->dw_buff_offt) * 100/handle->dw_dlen;
            os_printf(LM_APP, LL_ERR, "OTA download %d%%\n", handle->dw_percent);
        }
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_packet_flush(download_oper_t *handle)
{
    int ret;

    if (handle->dw_buff_offt != 0)
    {
        ret = drv_spiflash_erase(handle->dw_addr + handle->dw_offt, handle->dw_buff_size);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        ret = drv_spiflash_write(handle->dw_addr + handle->dw_offt, handle->dw_buff, handle->dw_buff_size);
        DOWNLOAD_RET_CHECK_RETURN(ret);

        handle->dw_offt += handle->dw_buff_offt;
        handle->dw_buff_offt = 0;
        memset(handle->dw_buff, 0xFF, handle->dw_buff_size);
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_package_head(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    unsigned int writelen = 0;
    unsigned int leftlen = 0;
    unsigned char *phead = (unsigned char *)&handle->dw_head;

    DOWNLOAD_RET_CHECK_RETURN(handle->dw_state != DOWNLOAD_HEAD_ST);
    leftlen = handle->dw_dlen - handle->dw_offt - handle->dw_buff_offt;
    writelen = leftlen > len ? len : leftlen;
    memcpy(phead + handle->dw_offt, data, writelen);
    handle->dw_offt += writelen;

    if (len > writelen || leftlen == writelen)
    {
        return download_packet_data(handle, data + writelen, len - writelen);
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_get_firmware_size(download_oper_t *handle)
{
    image_headinfo_t image_head;
    int ret;

    ret = drv_spiflash_read(handle->dw_paddr[DOWNLOAD_IMGA_PART], (unsigned char *)&image_head, sizeof(image_headinfo_t));
    DOWNLOAD_RET_CHECK_RETURN(ret);

    return image_head.data_size + image_head.xip_size + image_head.text_size + sizeof(image_headinfo_t);
}

static int download_packet_head_check(download_oper_t *handle)
{
    unsigned int filesize;
    unsigned int firmwaresize;

    if (memcmp(handle->dw_head.magic, "FotaPKG", strlen("FotaPKG")) != 0)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_MAGIC_ERR);
    }

    if (handle->dw_rmethod == DOWNLOAD_DIFF_METHOD && handle->dw_lmethod == DOWNLOAD_DUAL_METHOD)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_METH_ERR);
    }

    firmwaresize = handle->dw_head.firmware_size;
    if (handle->dw_head.firmware_size == 0)
    {
        firmwaresize = download_get_firmware_size(handle);
    }

    firmwaresize = firmwaresize > handle->dw_head.firmware_new_size ?
        firmwaresize : handle->dw_head.firmware_new_size;
    firmwaresize = OTA_SIZE_ALIGN_4K(firmwaresize);
    filesize = OTA_SIZE_ALIGN_4K(handle->dw_head.package_size);

    if (handle->dw_rmethod != DOWNLOAD_DUAL_METHOD && handle->dw_lmethod != DOWNLOAD_DUAL_METHOD)
    {
        if (firmwaresize + filesize > handle->dw_pdlen[DOWNLOAD_IMGA_PART] + handle->dw_pdlen[DOWNLOAD_IMGB_PART])
        {
            DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_SIZE_ERR);
        }

        if (handle->dw_rmethod == DOWNLOAD_DIFF_METHOD)
        {
            if (firmwaresize + filesize + OTA_STAT_PART_SIZE + OTA_BACKUP_PART_SIZE > handle->dw_pdlen[DOWNLOAD_IMGA_PART] + handle->dw_pdlen[DOWNLOAD_IMGB_PART])
            {
                DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_SIZE_ERR);
            }
        }
    }
    else
    {
        firmwaresize = (handle->dw_active_part == DOWNLOAD_OTA_PARTA) ? handle->dw_head.firmware_new_size :
            handle->dw_head.firmware_size;
        if (firmwaresize > handle->dw_pdlen[DOWNLOAD_IMGA_PART])
        {
            DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_SIZE_ERR);
        }
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_packet_complete(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    int ret;

    if (handle->dw_state == DOWNLOAD_HEAD_ST)
    {
        handle->dw_rmethod = (handle->dw_head.version >> 4) / 4;
        ret = download_packet_head_check(handle);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        if (handle->dw_rmethod == DOWNLOAD_DUAL_METHOD)
        {
            handle->dw_state = DOWNLOAD_BOOT_ST;
            if (handle->dw_head.boot_size != 0)
            {
                handle->dw_addr = handle->dw_paddr[DOWNLOAD_BOOT_PART];
                handle->dw_dlen = handle->dw_head.boot_size;
                handle->dw_offt = 0;
            }
        }
        else
        {
            int filesize = OTA_SIZE_ALIGN_4K(handle->dw_head.package_size);
            memcpy(handle->dw_buff, &handle->dw_head, sizeof(handle->dw_head));
            handle->dw_buff_offt += sizeof(handle->dw_head);

            handle->dw_dlen = handle->dw_head.package_size;
            handle->dw_offt = 0;
            handle->dw_state = DOWNLOAD_IMGE_ST;

            if (handle->dw_rmethod == DOWNLOAD_COMZ_METHOD && handle->dw_active_part == DOWNLOAD_OTA_PARTB)
            {
                handle->dw_addr = handle->dw_paddr[DOWNLOAD_IMGA_PART];
            }
            else if (handle->dw_rmethod == DOWNLOAD_DIFF_METHOD)
            {
                handle->dw_addr = handle->dw_paddr[DOWNLOAD_IMGB_PART] + handle->dw_pdlen[DOWNLOAD_IMGB_PART] -
                    filesize - OTA_BACKUP_PART_SIZE;;
            }
            else
            {
                handle->dw_addr = handle->dw_paddr[DOWNLOAD_IMGB_PART] + handle->dw_pdlen[DOWNLOAD_IMGB_PART] - filesize;
            }
        }

        if (len > 0)
        {
            ret = download_packet_data(handle, data, len);
            DOWNLOAD_RET_CHECK_RETURN(ret);
        }

        return DOWNLOAD_NONE_ERR;
    }

    if (handle->dw_state == DOWNLOAD_BOOT_ST)
    {
        handle->dw_addr = handle->dw_paddr[DOWNLOAD_IMGA_PART];
        handle->dw_dlen = handle->dw_head.firmware_size;
        handle->dw_offt = 0;
        handle->dw_state = DOWNLOAD_IMGA_ST;

        if (handle->dw_active_part == DOWNLOAD_OTA_PARTA)
            handle->dw_skip = 1;


        if (len > 0)
        {
            ret = download_packet_data(handle, data, len);
            DOWNLOAD_RET_CHECK_RETURN(ret);
        }

        return DOWNLOAD_NONE_ERR;
    }

    if (handle->dw_state == DOWNLOAD_IMGA_ST)
    {
        handle->dw_addr = handle->dw_paddr[DOWNLOAD_IMGB_PART];
        handle->dw_dlen = handle->dw_head.firmware_new_size;
        handle->dw_offt = 0;
        handle->dw_state = DOWNLOAD_IMGB_ST;

        if (handle->dw_active_part == DOWNLOAD_OTA_PARTB)
            handle->dw_skip = 1;

        if (len > 0)
        {
            ret = download_packet_data(handle, data, len);
            DOWNLOAD_RET_CHECK_RETURN(ret);
        }

        return DOWNLOAD_NONE_ERR;
    }

    DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_METH_ERR);
}

static int download_package_boot(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    unsigned int writelen = 0;
    unsigned int leftlen = 0;
    int ret;

    DOWNLOAD_RET_CHECK_RETURN(handle->dw_state != DOWNLOAD_BOOT_ST);
    DOWNLOAD_RET_CHECK_RETURN(handle->dw_dlen == 0);

    leftlen = handle->dw_dlen - handle->dw_offt - handle->dw_buff_offt;
    writelen = leftlen > len ? len : leftlen;
    ret = download_packet_flash(handle, data, writelen);
    DOWNLOAD_RET_CHECK_RETURN(ret);

    if (writelen == leftlen)
    {
        ret = download_packet_flush(handle);
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }

    if (len > writelen)
    {
        return download_packet_data(handle, data + writelen, len - writelen);
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_package_imge(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    unsigned int writelen = 0;
    unsigned int leftlen = 0;
    int ret;

    leftlen = handle->dw_dlen - handle->dw_offt - handle->dw_buff_offt;
    writelen = leftlen > len ? len : leftlen;

    if (handle->dw_skip == 0)
    {
        ret = download_packet_flash(handle, data, writelen);
        DOWNLOAD_RET_CHECK_RETURN(ret);

        if (writelen == leftlen)
        {
            ret = download_packet_flush(handle);
            DOWNLOAD_RET_CHECK_RETURN(ret);
        }

        if (len > writelen)
        {
            return download_packet_data(handle, data + writelen, len - writelen);
        }
    }
    else
    {
        handle->dw_offt += writelen;
        if (len > writelen)
        {
            handle->dw_skip = 0;
            ret = download_packet_data(handle, data + writelen, len - writelen);
            DOWNLOAD_RET_CHECK_RETURN(ret);
        }
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_packet_data(download_oper_t *handle, unsigned char *data, unsigned int len)
{
    int ret;

    if (handle->dw_dlen == 0 && handle->dw_state == DOWNLOAD_IDLE_ST)
    {
        handle->dw_dlen = sizeof(ota_package_head_t);
        handle->dw_state = DOWNLOAD_HEAD_ST;
    }

    if (handle->dw_offt == handle->dw_dlen)
    {
        ret = download_packet_complete(handle, data, len);
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }
    else if (handle->dw_state == DOWNLOAD_HEAD_ST)
    {
        ret = download_package_head(handle, data, len);
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }
    else if (handle->dw_state == DOWNLOAD_BOOT_ST)
    {
        ret = download_package_boot(handle, data, len);
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }
    else if (handle->dw_state >= DOWNLOAD_IMGE_ST && handle->dw_state <= DOWNLOAD_IMGB_ST)
    {
        ret = download_package_imge(handle, data, len);
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }
    else
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_STAT_ERR);
    }

    return DOWNLOAD_NONE_ERR;
}

static int download_mem_init(download_oper_t **handle)
{
    download_oper_t *ht = *handle;

    if (ht != NULL)
    {
        return DOWNLOAD_NONE_ERR;
    }

    ht = (download_oper_t *)os_malloc(sizeof(download_oper_t));
    if (ht == NULL)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_NOMEM_ERR);
    }
    memset(ht, 0, sizeof(download_oper_t));
    ht->dw_buff_size = 4096;

    ht->dw_buff = (unsigned char *)os_malloc(ht->dw_buff_size);
    if (ht->dw_buff == NULL)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_NOMEM_ERR);
    }

    memset(ht->dw_buff, 0, ht->dw_buff_size);
    *handle = ht;

    return DOWNLOAD_NONE_ERR;
}

static void download_mem_release(download_oper_t **handle)
{
    download_oper_t *ht = *handle;

    if (ht == NULL)
    {
        return;
    }

    if (ht->dw_status != DOWNLOAD_NONE_ERR)
    {
        download_operate_dump(ht);
    }

    if (ht->dw_buff)
    {
        os_free(ht->dw_buff);
        ht->dw_buff = NULL;
    }

    os_free(ht);
    *handle = NULL;
}

static int download_packet_done(download_oper_t *handle, ota_state_t *state)
{
    unsigned int crc;

    if (handle->dw_active_part == DOWNLOAD_OTA_PARTB && handle->dw_rmethod != DOWNLOAD_COMZ_METHOD)
    {
        os_printf(LM_APP, LL_INFO, "OTA:* * * active part B * * *\n");
        crc = ota_get_flash_crc(handle->dw_paddr[DOWNLOAD_IMGA_PART], handle->dw_head.firmware_size);
        if (crc != handle->dw_head.firmware_crc)
        {
            os_printf(LM_APP, LL_ERR, "OTA:crc not correct. crc:0x%08x\n", crc);
            DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_CRC_ERR);
        }
        state->updata_ab = OTA_AB_UPDATAFLAG;
    }
    else
    {
        unsigned int len = offsetof(ota_package_head_t, boot_size);
        os_printf(LM_APP, LL_INFO, "OTA:* * * active part A * * *\n");
        if (handle->dw_rmethod == DOWNLOAD_DUAL_METHOD)
        {
            crc = ota_get_flash_crc(handle->dw_paddr[DOWNLOAD_IMGB_PART], handle->dw_head.firmware_new_size);
            if(crc != handle->dw_head.firmware_new_crc)
            {
                os_printf(LM_APP, LL_ERR, "OTA:crc not correct. crc:0x%08x\n", crc);
                DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_CRC_ERR);
            }
            state->updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B;
        }
        else
        {
            crc = ota_get_flash_crc(handle->dw_addr + len, handle->dw_head.package_size - len);
            if(crc != handle->dw_head.package_crc)
            {
                os_printf(LM_APP, LL_ERR, "OTA:crc not correct. crc:0x%08x\n", crc);
                DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_CRC_ERR);
            }

            if (handle->dw_active_part == DOWNLOAD_OTA_PARTB && handle->dw_rmethod == DOWNLOAD_COMZ_METHOD)
            {
                state->updata_ab |= OTA_MOVE_CZ_A2B;
            }
        }
    }

    return DOWNLOAD_NONE_ERR;
}

int ota_firmware_check(unsigned int data_len, unsigned int crc)
{
    unsigned int calcrc;
    int alone = 1;
    int ret;

    ret = ota_init();
    if (ret == DOWNLOAD_INITD_ERR)
    {
        alone = 0;
    }
    else
    {
        alone = 1;
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }

    if (crc == 0 || g_ota_download->dw_lmethod == DOWNLOAD_DUAL_METHOD)
    {
        if (alone == 1)
        {
            download_mem_release(&g_ota_download);
        }
        return DOWNLOAD_NONE_ERR;
    }

    calcrc = ota_get_flash_crc(g_ota_download->dw_paddr[DOWNLOAD_IMGA_PART], data_len);
    if (calcrc != crc)
    {
        os_printf(LM_APP, LL_ERR, "OTA:crc 0x%x not match calc 0x%x\n", crc, calcrc);
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_CRC_ERR);
    }

    os_printf(LM_APP, LL_DBG, "OTA:crc check pass\n");
    if (alone == 1)
    {
        download_mem_release(&g_ota_download);
    }

    return DOWNLOAD_NONE_ERR;
}

int ota_get_flash_crc(unsigned int addr, unsigned int size)
{
    unsigned int crc = 0;
    unsigned int len = size;
    unsigned int readLen;
    int alone = 1;
    int ret;

    ret = ota_init();
    if (ret == DOWNLOAD_INITD_ERR)
    {
        alone = 0;
    }
    else
    {
        alone = 1;
        DOWNLOAD_RET_CHECK_RETURN(ret);
    }

    while (len > 0)
    {
        memset(g_ota_download->dw_buff, 0, g_ota_download->dw_buff_size);
        readLen = (len > g_ota_download->dw_buff_size) ? g_ota_download->dw_buff_size : len;
        ret = drv_spiflash_read(addr + size - len, g_ota_download->dw_buff, readLen);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        crc = ef_calc_crc32(crc, g_ota_download->dw_buff, readLen);
        len -= readLen;
    }

    if (alone == 1)
    {
        download_mem_release(&g_ota_download);
    }

    return crc;
}

int ota_init(void)
{
    int ret;

    if (g_ota_download == NULL)
    {
        ret = download_mem_init(&g_ota_download);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        ret = download_partition_init(g_ota_download);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        ret = download_get_active_part(g_ota_download);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        ret = download_get_lmethod(g_ota_download);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        return DOWNLOAD_NONE_ERR;
    }

    return DOWNLOAD_INITD_ERR;
}

int ota_write(unsigned char *data, unsigned int len)
{
    if (g_ota_download == NULL)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_INIT_ERR);
    }

    return download_packet_data(g_ota_download, data, len);
}

int ota_done(bool reset)
{
    ota_state_t state;
    unsigned int len;
    int ret;

    if (g_ota_download == NULL)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_INIT_ERR);
    }

    memset(&state, 0x0, sizeof(ota_state_t));
    memcpy(state.magic, "OTA", 3);
    state.update_flag = 1;
    state.len = sizeof(ota_state_t);
    state.patch_addr = g_ota_download->dw_addr;
    state.patch_size = g_ota_download->dw_head.package_size;

    DOWNLOAD_RET_CHECK_RETURN(g_ota_download->dw_dlen == 0);
    if (g_ota_download->dw_dlen != g_ota_download->dw_offt)
    {
        DOWNLOAD_RET_CHECK_RETURN(-DOWNLOAD_SIZE_ERR);
    }
    ret = download_packet_done(g_ota_download, &state);
    DOWNLOAD_RET_CHECK_RETURN(ret);

    len = offsetof(ota_state_t, patch_addr);
    state.crc = ef_calc_crc32(0, (char *)&state + len, sizeof(ota_state_t) - len);
    ret = drv_spiflash_erase(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART], g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_write(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART], (const unsigned char *)&state, g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_erase(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART] + g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2,
        g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_write(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART] + g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2,
        (const unsigned char *)&state, g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);

    os_printf(LM_APP, LL_INFO, "ota done successs, reset now\n");
    if (reset)
    {
        hal_system_reset(RST_TYPE_OTA);
    }

    download_mem_release(&g_ota_download);

    return DOWNLOAD_NONE_ERR;
}

int ota_confirm_update(void)
{
    unsigned int calcrc;
    ota_state_t *state;
    int len = offsetof(ota_state_t, patch_addr);
    int ret;

    ota_init();
    if (g_ota_download->dw_lmethod != DOWNLOAD_DUAL_METHOD)
    {
        download_mem_release(&g_ota_download);
        return DOWNLOAD_NONE_ERR;
    }

    ret = drv_spiflash_read(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART], g_ota_download->dw_buff,
        g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    state = (ota_state_t *)g_ota_download->dw_buff;
    calcrc = ef_calc_crc32(0, (char *)state + len, sizeof(ota_state_t) - len);
    if(calcrc != state->crc)
    {
        os_printf(LM_APP, LL_DBG, "OTA:master part crc error\n");
        ret = drv_spiflash_read(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART] + g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2,
            g_ota_download->dw_buff, g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
        DOWNLOAD_RET_CHECK_RETURN(ret);
        calcrc = ef_calc_crc32(0, (char *)state + len, sizeof(ota_state_t) - len);
        if(calcrc != state->crc)
        {
            os_printf(LM_APP, LL_DBG, "OTA:slave part crc error\n");
            os_printf(LM_APP, LL_INFO, "OTA:state error, run A\n");
            download_mem_release(&g_ota_download);
            return -DOWNLOAD_CRC_ERR;
        }
    }

    if(state->updata_ab & OTA_AB_DIRETC_START)
    {
        os_printf(LM_APP, LL_ERR, "OTA:Already confirmed\n");
        download_mem_release(&g_ota_download);
        return DOWNLOAD_NONE_ERR;
    }

    if(g_ota_download->dw_active_part == DOWNLOAD_OTA_PARTB)
    {
        state->updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B | OTA_AB_DIRETC_START;
    }
    else
    {
        state->updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_DIRETC_START;
    }
    os_printf(LM_APP, LL_INFO, "OTA:confirm part(%d) state:0x%02x\n", g_ota_download->dw_active_part, state->updata_ab);

    state->crc = ef_calc_crc32(0, (char *)state + len, sizeof(ota_state_t) - len);

    ret = drv_spiflash_erase(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART], g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_write(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART], g_ota_download->dw_buff,
        g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_erase(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART] + g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2,
        g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);
    ret = drv_spiflash_write(g_ota_download->dw_paddr[DOWNLOAD_OTAS_PART] + g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2,
        g_ota_download->dw_buff, g_ota_download->dw_pdlen[DOWNLOAD_OTAS_PART]/2);
    DOWNLOAD_RET_CHECK_RETURN(ret);

    download_mem_release(&g_ota_download);
    return DOWNLOAD_NONE_ERR;
}

#ifdef CONFIG_SPI_SERVICE
int ota_update_image(unsigned char *data, unsigned int len)
{
    ota_init();
    ota_write(data, len);

    if (g_ota_download->dw_dlen != 0 && g_ota_download->dw_dlen == g_ota_download->dw_offt)
    {
        if (g_ota_download->dw_rmethod == DOWNLOAD_DUAL_METHOD && g_ota_download->dw_state == DOWNLOAD_IMGA_ST)
        {
            return DOWNLOAD_NONE_ERR;
        }
        ota_done(1);
        download_mem_release(&g_ota_download);
    }

    return DOWNLOAD_NONE_ERR;
}
#endif
