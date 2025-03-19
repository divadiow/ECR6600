#include <stddef.h>
#include <string.h>
//#include "os.h"
#include "flash.h"
#include "easyflash.h"
#include "oshal.h"
#include "ota.h"
#include "hal_system.h"

static ota_opinfo_t g_ota_info;
static ota_package_head_t g_ota_package_head;

static int ota_get_local_update_method(void)
{
    image_headinfo_t image_head;
    unsigned int start_addr = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)?
		g_ota_info.partition_cpu_addr + g_ota_info.partition_cpu_len / 2 : g_ota_info.partition_cpu_addr;
//    unsigned int part_size = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)?
//		g_ota_info.partition_cpu_len / 2 : g_ota_info.partition_cpu_len;
	
    os_printf(LM_APP, LL_DBG, "OTA:start addr 0x%x\n", start_addr);
    drv_spiflash_read(start_addr, (unsigned char *)&image_head, sizeof(image_headinfo_t));
	
    switch (image_head.update_method)
    {
        case OTA_UPDATE_METHOD_AB:
            os_printf(LM_APP, LL_INFO, "OTA:get method is AB\n");
            break;
        case OTA_UPDATE_METHOD_CZ:
            os_printf(LM_APP, LL_INFO, "OTA:get method is CZ\n");
            break;
        case OTA_UPDATE_METHOD_DI:
            os_printf(LM_APP, LL_INFO, "OTA:get method is DI\n");
            break;
        default:
            os_printf(LM_APP, LL_ERR, "OTA:get unkown method 0x%02x\n", image_head.update_method);
            return -1;
    }

    g_ota_info.update_method = image_head.update_method;
    return 0;
}

static int ota_check_remote_head()
{
    if (memcmp(g_ota_package_head.magic, "FotaPKG", strlen("FotaPKG")) != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:Magic not correct\n");
		return -1;
    }

    if ((OTA_UPDATE_METHOD_AB == g_ota_info.update_method) && (OTA_UPDATE_METHOD_DI == g_ota_package_head.version))
    {
        os_printf(LM_APP, LL_ERR, "OTA:Does not support AB to DI\n");
        return -1;
    }
	else
	{
		switch(g_ota_info.update_method)
		{
			case OTA_UPDATE_METHOD_CZ:
				os_printf(LM_APP, LL_DBG, "OTA:local is CZ\n");
				break;
			case OTA_UPDATE_METHOD_DI:
				os_printf(LM_APP, LL_DBG, "OTA:local is DI\n");
				break;
			case OTA_UPDATE_METHOD_AB:
				os_printf(LM_APP, LL_DBG, "OTA:local is AB\n");
				break;
			default:
    			os_printf(LM_APP, LL_ERR, "OTA:local is unkown %d\n", g_ota_info.update_method);
        		return -1;
		}
		switch(g_ota_package_head.version)
		{
			case OTA_UPDATE_METHOD_CZ:
				os_printf(LM_APP, LL_DBG, "OTA:ota is CZ\n");
				break;
			case OTA_UPDATE_METHOD_DI:
				os_printf(LM_APP, LL_DBG, "OTA:ota is DI\n");
				break;
			case OTA_UPDATE_METHOD_AB:
				os_printf(LM_APP, LL_DBG, "OTA:ota is AB\n");
				break;
			default:
    			os_printf(LM_APP, LL_ERR, "OTA:ota is unkown %d\n", g_ota_package_head.version);
        		return -1;
		}
	}

    return 0;
}

static int ota_get_active_part(void)
{
	extern void *__etext1;
	#define FLASH_PHYSICAL_ADDRESS		0x40800000
    unsigned int text_addr = ((unsigned int)&__etext1) - FLASH_PHYSICAL_ADDRESS;

    os_printf(LM_APP, LL_DBG, "__etext1 = 0x%x\n", text_addr);
	
    if (text_addr > g_ota_info.partition_cpu_addr + g_ota_info.partition_cpu_len / 2)
    {
        g_ota_info.active_part = OTA_UPDATE_ACTIVE_PARTB;
        os_printf(LM_APP, LL_INFO, "OTA:* * * active part B * * *\n");
        return 0;
    }

    g_ota_info.active_part = OTA_UPDATE_ACTIVE_PARTA;
    os_printf(LM_APP, LL_INFO, "OTA:* * * active part A * * *\n");
    return 0;
}

/* just used for cz mode to get source firmware size */
static int ota_get_firmware_size(void)
{
    image_headinfo_t image_head;
    drv_spiflash_read(g_ota_info.partition_cpu_addr, (unsigned char *)&image_head, sizeof(image_headinfo_t));
    return image_head.data_size + image_head.xip_size + image_head.text_size + sizeof(image_headinfo_t);
}

int ota_get_flash_crc(unsigned int addr, unsigned int size)
{
    unsigned int crc = 0;
    unsigned char *buff = (unsigned char *)os_calloc(1, OTA_STAT_PART_SIZE);
    unsigned int len = size;
    unsigned int readLen;

    if (buff == NULL)
    {
        os_printf(LM_APP, LL_ERR, "OTA:no mem left for ota flash\n");
        return -1;
    }

    //os_printf(LM_APP, LL_INFO, "flash crc from addr 0x%x, size 0x%x\n", addr, size);
    while (len > 0)
    {
        memset(buff, 0, OTA_STAT_PART_SIZE);
        readLen = (len > OTA_STAT_PART_SIZE) ? OTA_STAT_PART_SIZE : len;
        drv_spiflash_read(addr + size - len, buff, readLen);
        crc = ef_calc_crc32(crc, buff, readLen);
        len -= readLen;
    }

    os_free(buff);

    return crc;
}


static int ota_flash_space_erase(unsigned int addr, unsigned int len)
{
    unsigned int eraselen;
    unsigned int eraseaddr;

    if (g_ota_info.erase_offset >= addr + len)
    {
        return 0;
    }

    if (g_ota_info.erase_offset >= addr)
    {
        eraseaddr = g_ota_info.erase_offset;
        eraselen = OTA_SIZE_ALIGN_4K(addr + len - g_ota_info.erase_offset);
        g_ota_info.erase_offset += eraselen;
    }
    else
    {
        os_printf(LM_APP, LL_ERR, "OTA:erase offset:0x%x addr:0x%x error range\n", g_ota_info.erase_offset, addr);
        return -1;
    }

    if ((eraseaddr < g_ota_info.image_start_addr) || (eraseaddr + eraselen > g_ota_info.image_end_addr))
    {
        os_printf(LM_APP, LL_ERR, "OTA:erase addr:0x%x len:0x%x error range\n", eraseaddr, eraselen);
        return -1;
    }

    if ((eraseaddr % OTA_STAT_PART_SIZE) || (eraselen % OTA_STAT_PART_SIZE))
    {
        os_printf(LM_APP, LL_ERR, "OTA:erase addr:0x%x len:0x%x not align 4K\n", eraseaddr, eraselen);
        return -1;
    }

    //os_printf(LM_APP, LL_ERR, "erase addr:0x%x len:0x%x\n", eraseaddr, eraselen);

    return drv_spiflash_erase(eraseaddr, eraselen);
}

static int ota_confirm_update_area_ab()
{
    unsigned int part_addr = g_ota_info.partition_cpu_addr;
    unsigned int part_size = g_ota_info.partition_cpu_len;
    unsigned int firmware_max_len;

    g_ota_info.image_start_addr = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTA) ? 
		(part_addr + part_size / 2) : part_addr;
    g_ota_info.erase_offset = g_ota_info.image_start_addr;
    g_ota_info.image_end_addr = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTA) ? 
		(part_addr + part_size) : (part_addr + part_size / 2);
    g_ota_info.image_size = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTA) ? 
		g_ota_package_head.firmware_new_size: g_ota_package_head.firmware_size;
    g_ota_info.image_offset = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTA) ? 
		sizeof(g_ota_package_head) + g_ota_package_head.boot_size + g_ota_package_head.firmware_size : 
		sizeof(g_ota_package_head) + g_ota_package_head.boot_size;

    firmware_max_len = (g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTA) ? 
		g_ota_package_head.firmware_new_size : g_ota_package_head.firmware_size;
	
    if (firmware_max_len > part_size / 2)
    {
        os_printf(LM_APP, LL_ERR, "OTA:master size:0x%x not enough, filelen:0x%x\n", part_size / 2, firmware_max_len);
        return -1;
    }


	// uboot info
    if (partion_info_get(PARTION_NAME_BOOT, &part_addr, &part_size) != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:can not get %s info\n", PARTION_NAME_BOOT);
        return -1;
    }
    g_ota_info.boot_start_addr = part_addr;
    g_ota_info.boot_end_addr = part_addr + part_size;
    if (g_ota_package_head.boot_size > part_size)
    {
        os_printf(LM_APP, LL_ERR, "OTA:boot 0x%x is too big than part size 0x%x\n", g_ota_package_head.boot_size, part_size);
        return -1;
    }
    if (g_ota_package_head.boot_size > 0) {
        os_printf(LM_APP, LL_ERR, "OTA:AB erase boot addr 0x%x with size 0x%x\n", 
			g_ota_info.boot_start_addr, g_ota_info.boot_end_addr - g_ota_info.boot_start_addr);
        drv_spiflash_erase(g_ota_info.boot_start_addr, g_ota_info.boot_end_addr - g_ota_info.boot_start_addr);
        g_ota_info.boot_size = g_ota_package_head.boot_size;
        g_ota_info.boot_offset = sizeof(g_ota_package_head);
    }

    return 0;
}

static int ota_confirm_update_area_diff()
{
    unsigned int master_part_addr = g_ota_info.partition_cpu_addr;
    unsigned int master_part_size = g_ota_info.partition_cpu_len;
    unsigned int firmware_max_len;
    unsigned int file_align_size;

    if (g_ota_package_head.firmware_size == 0)
    {
        g_ota_package_head.firmware_size = ota_get_firmware_size();
    }

    file_align_size = OTA_SIZE_ALIGN_4K(g_ota_package_head.package_size);
    firmware_max_len = (g_ota_package_head.firmware_size > g_ota_package_head.firmware_new_size) ? 
		g_ota_package_head.firmware_size : g_ota_package_head.firmware_new_size;
    firmware_max_len = OTA_SIZE_ALIGN_4K(firmware_max_len);

    if (g_ota_package_head.version == OTA_UPDATE_METHOD_DI)
    {
		os_printf(LM_APP, LL_DBG, "OTA:for DI\n");
        if (file_align_size + firmware_max_len + OTA_STAT_PART_SIZE + OTA_BACKUP_PART_SIZE > master_part_size)  //OTA_STAT_PART_SIZE:move part
        {
            os_printf(LM_APP, LL_ERR, "OTA:master size:0x%x not enough, firmwarelen:0x%x, filealignlen:0x%x\n", 
				master_part_size, firmware_max_len, file_align_size);
            return -1;
        }
        g_ota_info.image_start_addr = master_part_addr + master_part_size - file_align_size - OTA_BACKUP_PART_SIZE;
    	g_ota_info.image_end_addr = master_part_addr + master_part_size;
	} 
	else 
	{
		if (g_ota_info.update_method == OTA_UPDATE_METHOD_AB && g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)
		{
			os_printf(LM_APP, LL_DBG, "OTA:for AB(B)->CZ\n");
			if (file_align_size > master_part_size/2)
			{
				os_printf(LM_APP, LL_ERR, "OTA:AB size:0x%x not enough, firmwarelen:0x%x, filealignlen:0x%x\n", 
					master_part_size/2, firmware_max_len, file_align_size);
				return -1;
			}
			g_ota_info.image_start_addr = master_part_addr;
			g_ota_info.image_end_addr = master_part_addr + master_part_size/2;
		}
		else
		{
			os_printf(LM_APP, LL_DBG, "OTA:for CZ\n");
			if (file_align_size + firmware_max_len > master_part_size)
			{
				os_printf(LM_APP, LL_ERR, "OTA:master size:0x%x not enough, firmwarelen:0x%x, filealignlen:0x%x\n", 
					master_part_size, firmware_max_len, file_align_size);
				return -1;
			}
			g_ota_info.image_start_addr = master_part_addr + master_part_size - file_align_size;
    		g_ota_info.image_end_addr = master_part_addr + master_part_size;
		}
    }

    g_ota_info.erase_offset = g_ota_info.image_start_addr;
	os_printf(LM_APP, LL_DBG, "OTA:start write addr 0x%08x\n", g_ota_info.image_start_addr);
	os_printf(LM_APP, LL_DBG, "OTA:file_align_size 0x%08x\n", file_align_size);
	os_printf(LM_APP, LL_DBG, "OTA:master_part_addr 0x%08x\n", master_part_addr);
	os_printf(LM_APP, LL_DBG, "OTA:master_part_size 0x%08x\n", master_part_size);

    return 0;
}


static int ota_write_flash_ab(unsigned char *data, unsigned int len)
{
    unsigned int length;

    /** support flash boot */
    if (g_ota_info.offset < g_ota_info.boot_offset + g_ota_info.boot_size)
    {
        if (g_ota_info.offset + len <= g_ota_info.boot_offset + g_ota_info.boot_size)
        {
            os_printf(LM_APP, LL_INFO, "OTA:write boot addr 0x%x offset 0x%x size 0x%x\n", 
				g_ota_info.boot_start_addr + g_ota_info.offset - g_ota_info.boot_offset, g_ota_info.offset, len);
            drv_spiflash_write(g_ota_info.boot_start_addr + g_ota_info.offset - g_ota_info.boot_offset, data, len);
            g_ota_info.offset += len;

            return 0;
        }
        else
        {
            length = g_ota_info.boot_offset + g_ota_info.boot_size - g_ota_info.offset;
            os_printf(LM_APP, LL_INFO, "OTA:write boot left addr 0x%x offset 0x%x size 0x%x\n", 
				g_ota_info.boot_start_addr + g_ota_info.offset - g_ota_info.boot_offset, g_ota_info.offset, length);
            drv_spiflash_write(g_ota_info.boot_start_addr + g_ota_info.offset - g_ota_info.boot_offset, data, length);
            g_ota_info.offset += length;

            if (g_ota_info.offset == g_ota_info.image_offset)
            {
                os_printf(LM_APP, LL_INFO, "OTA:write first Aimage addr 0x%x offset 0x%x size 0x%x\n", 
					g_ota_info.image_start_addr, g_ota_info.image_offset, len - length);
                ota_flash_space_erase(g_ota_info.image_start_addr, len - length);
                drv_spiflash_write(g_ota_info.image_start_addr, (unsigned char *)data + length, len - length);
            }
            g_ota_info.offset += len - length;

            return 0;
        }
    }

    if (g_ota_info.offset >= g_ota_info.image_offset)
    {
        /** flash image bin */
        if (g_ota_info.offset >= g_ota_info.image_offset + g_ota_info.image_size)
        {
            //os_printf(LM_APP, LL_INFO, "extraB offset 0x%x\n", g_ota_info.offset);
            g_ota_info.offset += len;
            return 0;
        }

        if (g_ota_info.offset + len > g_ota_info.image_offset + g_ota_info.image_size)
        {
            length = g_ota_info.image_offset + g_ota_info.image_size - g_ota_info.offset;
            //os_printf(LM_APP, LL_INFO, "write Aimage last addr 0x%x offset 0x%x size 0x%x\n", g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, g_ota_info.offset, length);
            ota_flash_space_erase(g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, length);
            drv_spiflash_write(g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, data, length);
            g_ota_info.offset += len;
            return 0;
        }

        //os_printf(LM_APP, LL_INFO, "write image addr 0x%x offset 0x%x size 0x%x\n", g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, g_ota_info.offset, len);
        ota_flash_space_erase(g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, len);
        drv_spiflash_write(g_ota_info.image_start_addr + g_ota_info.offset - g_ota_info.image_offset, data, len);
        g_ota_info.offset += len;
    }
    else
    {
        g_ota_info.offset += len;
        if (g_ota_info.offset > g_ota_info.image_offset)
        {
            length = g_ota_info.offset - g_ota_info.image_offset;
            //os_printf(LM_APP, LL_INFO, "write Bimage first addr 0x%x offset 0x%x size 0x%x\n", g_ota_info.image_start_addr, g_ota_info.image_offset, length);
            ota_flash_space_erase(g_ota_info.image_start_addr, length);
            drv_spiflash_write(g_ota_info.image_start_addr, (unsigned char *)data + len - length, length);
        }
        else
        {
            //os_printf(LM_APP, LL_INFO, "extraA offset 0x%x\n", g_ota_info.offset);
        }
    }

    return 0;
}

static int ota_write_flash_diff(unsigned char *data, unsigned int len)
{
    //os_printf(LM_APP, LL_INFO, "write addr 0x%x offset 0x%x size 0x%x\n", g_ota_info.image_start_addr + g_ota_info.offset, g_ota_info.offset, len);
    ota_flash_space_erase(g_ota_info.image_start_addr + g_ota_info.offset, len);
    if (drv_spiflash_write(g_ota_info.image_start_addr + g_ota_info.offset, data, len) != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:flash write interface error");
        return -1;
    }
    g_ota_info.offset += len;

    return 0;
}



static int ota_write_done_ab()
{
    ota_state_t state;
	unsigned int crc;
    memset(&state, 0x0, sizeof(state));
    memcpy(state.magic, "OTA", 3);
    state.update_flag = 1;
    state.len = sizeof(ota_state_t);
    state.patch_addr = g_ota_info.image_start_addr;
    state.patch_size = g_ota_package_head.package_size;

	if(g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)
	{
        os_printf(LM_APP, LL_INFO, "OTA:* * * active part B * * *\n");
		crc = ota_get_flash_crc(g_ota_info.image_start_addr, g_ota_package_head.firmware_size);
		if(crc != g_ota_package_head.firmware_crc)
		{
			os_printf(LM_APP, LL_ERR, "OTA:crc not correct. c_crc:0x%08x, d_crc:0x%08x\n", crc, g_ota_package_head.firmware_crc);
			return -1;
		}
		state.updata_ab = OTA_AB_UPDATAFLAG;
	}
	else
	{
        os_printf(LM_APP, LL_INFO, "OTA:* * * active part A * * *\n");
		crc = ota_get_flash_crc(g_ota_info.image_start_addr, g_ota_package_head.firmware_new_size);
		if(crc != g_ota_package_head.firmware_new_crc)
		{
			os_printf(LM_APP, LL_ERR, "OTA:crc not correct. c_crc:0x%08x, d_crc:0x%08x\n", crc, g_ota_package_head.firmware_new_crc);
			return -1;
		}
		state.updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B;
	}
	os_printf(LM_APP, LL_INFO, "OTA:write state:0x%02x\n", g_ota_info.active_part);

    int len = offsetof(ota_state_t, patch_addr);
    state.crc = ef_calc_crc32(0, (char *)&state + len, sizeof(state) - len);
    os_printf(LM_APP, LL_DBG, "OTA:offset=%x data_len=%x\n", g_ota_info.offset, g_ota_package_head.package_size);
    os_printf(LM_APP, LL_DBG, "OTA:state crc=%x\n", state.crc);

    len = g_ota_info.partition_ota_status_len / 2;
    drv_spiflash_erase(g_ota_info.partition_ota_status_addr, len);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr, (unsigned char *)&state, sizeof(state));
    drv_spiflash_erase(g_ota_info.partition_ota_status_addr + len, len);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr + len, (unsigned char *)&state, sizeof(state));
    return 0;
}

static int ota_write_done_diff()
{
    ota_state_t state;
    unsigned int len = offsetof(ota_package_head_t, boot_size);
    unsigned int crc = ota_get_flash_crc(g_ota_info.image_start_addr + len, g_ota_package_head.package_size - len);

    if (crc != g_ota_package_head.package_crc)
    {
        os_printf(LM_APP, LL_ERR, "OTA:crc not correct. crc 0x%x localcrc 0x%x\n", crc, g_ota_package_head.package_crc);
        return -1;
    }
    os_printf(LM_APP, LL_INFO, "OTA:package crc check pass. crc 0x%x localcrc 0x%x\n", crc, g_ota_package_head.package_crc);

    memset(&state, 0x0, sizeof(state));
    state.len = sizeof(ota_state_t);
    memcpy(state.magic, "OTA", 3);
    state.update_flag = 1;
    state.patch_addr = g_ota_info.image_start_addr;
    state.patch_size = g_ota_package_head.package_size;
	
	if(g_ota_info.update_method == OTA_UPDATE_METHOD_AB && g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)
	{
		state.updata_ab |= OTA_MOVE_CZ_A2B;
	}
	os_printf(LM_APP, LL_DBG, "ota_write_done_diff state:0x%02x\n", g_ota_info.active_part);

    len = offsetof(ota_state_t, patch_addr);
    state.crc = ef_calc_crc32(0, (char *)&state + len, sizeof(state) - len);
    os_printf(LM_APP, LL_INFO, "OTA:offset=%x data_len=%x\n", g_ota_info.offset, g_ota_package_head.package_size);
    os_printf(LM_APP, LL_INFO, "OTA:state crc=%x\n", state.crc);

    len = g_ota_info.partition_ota_status_len / 2;
    drv_spiflash_erase(g_ota_info.partition_ota_status_addr, len);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr, (unsigned char *)&state, sizeof(state));
	drv_spiflash_erase(g_ota_info.partition_ota_status_addr + len, len);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr + len, (unsigned char *)&state, sizeof(state));
    return 0;
}




/* * * * * * * * * * * * * * * * * * * * * *
 * OTA API
 * * * * * * * * * * * * * * * * * * * * * */

int ota_firmware_check(unsigned int data_len, unsigned int crc)
{
    unsigned int calcrc;

    /* just check diff update mode */
	if (g_ota_info.update_method != OTA_UPDATE_METHOD_DI)
	{
    	return 0;
	}

    if (crc == 0)
    {
        return 0;
    }

    calcrc = ota_get_flash_crc(g_ota_info.partition_cpu_addr, data_len);
    if (calcrc != crc)
    {
        os_printf(LM_APP, LL_ERR, "OTA:crc 0x%x not match calc 0x%x\n", crc, calcrc);
        return -1;
    }

    os_printf(LM_APP, LL_DBG, "OTA:crc check pass\n");
    return 0;
}

int ota_init(void)
{
	unsigned int get_addr;
	unsigned int get_len;
    memset(&g_ota_info, 0, sizeof(g_ota_info));
    memset(&g_ota_package_head, 0, sizeof(g_ota_package_head));
	
    if (partion_info_get(PARTION_NAME_CPU, &get_addr, &get_len) != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:can not get %s info\n", PARTION_NAME_CPU);
        return -1;
    }
	else
	{
		g_ota_info.partition_cpu_addr = get_addr;
		g_ota_info.partition_cpu_len = get_len;
	}
 
 	if (partion_info_get(PARTION_NAME_OTA_STATUS, &get_addr, &get_len) != 0)
 	{
		os_printf(LM_APP, LL_ERR, "OTA:can not get %s info, does not support OTA\n", PARTION_NAME_OTA_STATUS);
		return -1;
//        os_printf(LM_APP, LL_ERR, "OTA:can not get %s info, use the last 8K of CPU partition\n", PARTION_NAME_CPU);
//		g_ota_info.partition_ota_status_addr = g_ota_info.partition_cpu_addr + g_ota_info.partition_cpu_len - 2*OTA_STAT_PART_SIZE;
//		g_ota_info.partition_ota_status_len = 2*OTA_STAT_PART_SIZE;
 	}
	else
	{
		g_ota_info.partition_ota_status_addr = get_addr;
		g_ota_info.partition_ota_status_len = get_len;
	}	

    if (ota_get_active_part() != 0)
    {
        return -1;
    }

	if (ota_get_local_update_method() != 0)
	{
		return -1;
	}

    g_ota_info.init = 1;
    return 0;
}

int ota_write(unsigned char *data, unsigned int len)
{
    static unsigned int progress = 0;
    int ret;

    if (g_ota_info.init == 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:please do ota init\n");
        return -1;
    }

    if (g_ota_info.head_frame_done != 1)
    {
        if (len < sizeof(ota_package_head_t))
        {
            os_printf(LM_APP, LL_ERR, "OTA:head_frame len=%d head_size=%d\n", len, sizeof(ota_package_head_t));
            return -1;
        }

        memcpy(&g_ota_package_head, data, sizeof(ota_package_head_t));

        if (ota_check_remote_head() != 0)
            return -1;

        switch (g_ota_package_head.version)
        {
            case OTA_UPDATE_METHOD_AB:
                if (ota_confirm_update_area_ab() != 0)
                    return -1;
                g_ota_info.offset = sizeof(ota_package_head_t);
                ota_write_flash_ab(data + sizeof(ota_package_head_t), len - sizeof(ota_package_head_t));
                break;
            case OTA_UPDATE_METHOD_CZ:
            case OTA_UPDATE_METHOD_DI:
                if (ota_confirm_update_area_diff() != 0)
                    return -1;
                if (ota_write_flash_diff(data, len) != 0)
                    return -1;
                break;
            default:
                os_printf(LM_APP, LL_ERR, "OTA:unkown method 0x%02x\n", g_ota_info.update_method);
                return -1;
        }

        g_ota_info.head_frame_done = 1;

        return 0;
    }

    switch (g_ota_package_head.version)
    {
        case OTA_UPDATE_METHOD_AB:
            ret = ota_write_flash_ab(data, len);
            break;
        case OTA_UPDATE_METHOD_CZ:
        case OTA_UPDATE_METHOD_DI:
            ret = ota_write_flash_diff(data, len);
            break;
        default:
            os_printf(LM_APP, LL_ERR, "OTA:unkown method %d\n", g_ota_info.update_method);
            ret = -1;
            break;
    }

	int new_progress = (int)(g_ota_info.offset * 100 / g_ota_package_head.package_size);
    if ((progress+4) <= new_progress) 
	{
        progress = new_progress;
        os_printf(LM_APP, LL_INFO, "OTA:ota download firmware %d%%\n", progress);
    }

    return ret;
}


int ota_done(bool reset)
{
    int ret;

    if (g_ota_info.init == 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:please do ota init\n");
        return -1;
    }

    if ((g_ota_package_head.package_size == 0) || (g_ota_info.offset != g_ota_package_head.package_size))
    {
        os_printf(LM_APP, LL_ERR, "OTA:ota download not completely. fileLen=0x%x downloadLen=0x%x\n", g_ota_package_head.package_size,  g_ota_info.offset);
        return -1;
    }

    switch (g_ota_package_head.version)
    {
        case OTA_UPDATE_METHOD_AB:
            ret = ota_write_done_ab();
            os_printf(LM_APP, LL_DBG, "OTA:AB ota complete flash\n");
            break;
        case OTA_UPDATE_METHOD_CZ:
        case OTA_UPDATE_METHOD_DI:
            os_printf(LM_APP, LL_DBG, "OTA:ota complete flash\n");
            ret = ota_write_done_diff();
            break;
        default:
            os_printf(LM_APP, LL_ERR, "OTA:unkown method %d\n", g_ota_info.update_method);
            ret = -1;
            break;
    }

    if (ret == 0)
    {
        os_printf(LM_APP, LL_INFO, "ota done successs, reset now\n\n");
        if (reset) {
            hal_system_reset(RST_TYPE_OTA);
        	//os_printf(LM_APP, LL_INFO, "* * * * * * * * * * * * * * * * * * * * * *\n");
        }
    }

    return ret;
}

int ota_confirm_update(void)
{
	unsigned int calcrc;
    ota_state_t state;
    int len = offsetof(ota_state_t, patch_addr);
	
	ota_init();
	if(g_ota_info.update_method != OTA_UPDATE_METHOD_AB)
	{
		return -1;
	}
	
	drv_spiflash_read(g_ota_info.partition_ota_status_addr, (unsigned char *)&state, sizeof(state));
	/* master partstate crc check */
    calcrc = ef_calc_crc32(0, (char *)&state + len, sizeof(state) - len);
	if(calcrc != state.crc)
	{
		os_printf(LM_APP, LL_DBG, "OTA:master part crc error\n");
		drv_spiflash_read(g_ota_info.partition_ota_status_addr+g_ota_info.partition_ota_status_len/2, (unsigned char *)&state, sizeof(state));
		/* slave partstate crc check */
	    calcrc = ef_calc_crc32(0, (char *)&state + len, sizeof(state) - len);
		if(calcrc != state.crc)
		{
			os_printf(LM_APP, LL_DBG, "OTA:slave part crc error\n");
			os_printf(LM_APP, LL_INFO, "OTA:state error, run A\n");
			return -1;
		}
	}

	if(state.updata_ab & OTA_AB_DIRETC_START)
	{
		os_printf(LM_APP, LL_ERR, "OTA:Already confirmed\n");
		return 0;
	}
	
	if(g_ota_info.active_part == OTA_UPDATE_ACTIVE_PARTB)
	{
		state.updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B | OTA_AB_DIRETC_START;
	}
	else
	{
		state.updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_DIRETC_START;
	}
	os_printf(LM_APP, LL_INFO, "OTA:confirm state:0x%02x\n", state.updata_ab);

    state.crc = ef_calc_crc32(0, (char *)&state + len, sizeof(state) - len);
    //os_printf(LM_APP, LL_DBG, "OTA:offset=%x data_len=%x\n", g_ota_info.offset, g_ota_package_head.package_size);
    //os_printf(LM_APP, LL_DBG, "OTA:state crc=%x\n", state.crc);

    drv_spiflash_erase(g_ota_info.partition_ota_status_addr, OTA_STAT_PART_SIZE);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr, (unsigned char *)&state, sizeof(state));
    drv_spiflash_erase(g_ota_info.partition_ota_status_addr + OTA_STAT_PART_SIZE, OTA_STAT_PART_SIZE);
    drv_spiflash_write(g_ota_info.partition_ota_status_addr + OTA_STAT_PART_SIZE, (unsigned char *)&state, sizeof(state));
    return 0;
}


//int ota_get_header_size(void)
//{
//    return sizeof(ota_package_head_t);
//}
//
//int ota_write_header(unsigned char *data, unsigned int len)
//{
//    if (g_ota_info.init == 0)
//    {
//        os_printf(LM_APP, LL_ERR, "OTA:please do ota init\n");
//        return -1;
//    }
//
//    if (len != sizeof(ota_package_head_t))
//    {
//        os_printf(LM_APP, LL_ERR, "OTA:len=%d headersize=%d\n", len, sizeof(ota_package_head_t));
//        return -1;
//    }
//    memcpy(&g_ota_package_head, data, sizeof(ota_package_head_t));
//    if (ota_check_remote_head() != 0)
//        return -1;
//
//    switch (g_ota_info.update_method)
//    {
//        case OTA_UPDATE_METHOD_AB:
//            if (ota_confirm_update_area_ab() != 0)
//                return -1;
//            break;
//        case OTA_UPDATE_METHOD_CZ:
//        case OTA_UPDATE_METHOD_DI:
//            if (ota_confirm_update_area_diff() != 0)
//                return -1;
//            drv_spiflash_write(g_ota_info.image_start_addr, data, len);
//            break;
//        default:
//            os_printf(LM_APP, LL_ERR, "OTA:unkown method %d\n", g_ota_info.update_method);
//            return -1;;
//    }
//
//    return 0;
//}
//
//int ota_get_boot_size()
//{
//    if (g_ota_info.update_method == OTA_UPDATE_METHOD_AB)
//    {
//        return g_ota_package_head.boot_size;
//    }
//
//    return 0;
//}
//
//int ota_write_boot(unsigned char *data, unsigned int len)
//{
//    if (g_ota_info.offset < g_ota_package_head.boot_size)
//    {
//        drv_spiflash_write(g_ota_info.boot_start_addr + g_ota_info.boot_offset, data, len);
//        g_ota_info.boot_offset += len;
//    }
//
//    return 0;
//}
//
//int ota_write_image(unsigned char *data, unsigned int len)
//{
//    static unsigned int progress = 0;
//    if (g_ota_info.boot_offset != g_ota_package_head.boot_size)
//    {
//        os_printf(LM_APP, LL_ERR, "OTA:please check boot 0x%x size 0x%x\n", g_ota_info.boot_offset, g_ota_package_head.boot_size);
//        return -1;
//    }
//
//    if (g_ota_info.offset < g_ota_info.image_size)
//    {
//        drv_spiflash_write(g_ota_info.image_start_addr + g_ota_info.image_offset, data, len);
//        g_ota_info.image_offset += len;
//    }
//
//    if (progress != g_ota_info.offset * 100 / (g_ota_info.boot_offset + g_ota_info.image_offset)) {
//        progress = g_ota_info.offset * 100 / (g_ota_info.boot_offset + g_ota_info.image_offset);
//        os_printf(LM_APP, LL_INFO, "OTA:download firmware(%d%)\n", g_ota_info.offset * 100 / (g_ota_info.boot_offset + g_ota_info.image_offset));
//    }
//
//    return 0;
//}
//
//int ota_write_done()
//{
//    int ret;
//
//    if (g_ota_info.init == 0)
//    {
//        os_printf(LM_APP, LL_ERR, "OTA:please do ota init\n");
//        return -1;
//    }
//
//    if ((g_ota_package_head.package_size == 0) || (g_ota_info.offset != g_ota_info.boot_offset + g_ota_info.image_offset))
//    {
//        os_printf(LM_APP, LL_ERR, "OTA:ota download not completely. fileLen 0x%x downbootlen 0x%x downimagelen 0x%x\n",
//            g_ota_package_head.package_size,  g_ota_info.boot_offset, g_ota_info.image_offset);
//        return -1;
//    }
//
//    switch (g_ota_info.update_method)
//    {
//        case OTA_UPDATE_METHOD_AB:
//            ret = ota_write_done_ab();
//            os_printf(LM_APP, LL_ERR, "OTA:AB ota complete flash\n");
//            break;
//        case OTA_UPDATE_METHOD_CZ:
//        case OTA_UPDATE_METHOD_DI:
//            ret = ota_write_done_diff();
//            break;
//        default:
//            os_printf(LM_APP, LL_ERR, "OTA:unkown method %d\n", g_ota_info.update_method);
//            ret = -1;
//            break;
//    }
//
//    return ret;
//}

#ifdef CONFIG_ZERATUL_APP
int ota_update_image(unsigned char *data, unsigned int len)
{
    if (g_ota_info.init == 0)
    {
        ota_init();
    }

    ota_write(data, len);

    if ((g_ota_package_head.package_size != 0) && (g_ota_info.offset == g_ota_package_head.package_size))
    {
        ota_done(1);
    }
	return 0;
}
#endif



