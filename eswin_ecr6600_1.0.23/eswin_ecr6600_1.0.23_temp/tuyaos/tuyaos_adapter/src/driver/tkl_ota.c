/**
 * @file tuya_os_adapt_ota.c
 * @brief ota底层操作接口
 * 
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 * 
 */
#include <string.h>
#include "tkl_flash.h" 
#include "tkl_ota.h"
#include "tuya_error_code.h"
#include "tkl_system.h"

#include "easyflash.h"

/***********************************************************
*************************micro define***********************
***********************************************************/


/***********************************************************
*************************variable define********************
***********************************************************/


/***********************************************************
*************************function define********************
***********************************************************/
#define TUYA_OTA_UPGRADE 1

unsigned int patch_size = 0;

#if TUYA_OTA_UPGRADE
#define TUYA_OTA_PATH  0x4d4d4d
#define PARTITION_NAME_CPU				"cpu"
#define PARTITION_NAME_OTA_STATUS		"ota_status"

unsigned int write_flash_data_len = 0;
unsigned int erase_data_len = 0;
unsigned int read_path_start_addr = 0;
unsigned int read_addr_mag = 0;
unsigned int read_addr_len = 0;
unsigned int erase_offset = 0;

#define RST_TYPE_OTA 4
#define BLOCK_SIZE    1 * 4096
#define CRC32_INT_VALUE     0xFFFFFFFFUL
#define CRC32_XOR_VALUE     0xFFFFFFFFUL
#define OTA_SIZE_ALIGN_4K(x)    ((x + 0xFFF) & (~0xFFF))
static int start_ota_data  = 0;
unsigned int path_bin_crc_new = 0;
static unsigned int download_progress = 0;

/* CRC-32/IEEE 常数表 */
static const unsigned int s_crc32[256] =
{
    0x00000000UL, 0x77073096UL, 0xEE0E612CUL, 0x990951BAUL,
    0x076DC419UL, 0x706AF48FUL, 0xE963A535UL, 0x9E6495A3UL,
    0x0EDB8832UL, 0x79DCB8A4UL, 0xE0D5E91EUL, 0x97D2D988UL,
    0x09B64C2BUL, 0x7EB17CBDUL, 0xE7B82D07UL, 0x90BF1D91UL,
    0x1DB71064UL, 0x6AB020F2UL, 0xF3B97148UL, 0x84BE41DEUL,
    0x1ADAD47DUL, 0x6DDDE4EBUL, 0xF4D4B551UL, 0x83D385C7UL,
    0x136C9856UL, 0x646BA8C0UL, 0xFD62F97AUL, 0x8A65C9ECUL,
    0x14015C4FUL, 0x63066CD9UL, 0xFA0F3D63UL, 0x8D080DF5UL,
    0x3B6E20C8UL, 0x4C69105EUL, 0xD56041E4UL, 0xA2677172UL,
    0x3C03E4D1UL, 0x4B04D447UL, 0xD20D85FDUL, 0xA50AB56BUL,
    0x35B5A8FAUL, 0x42B2986CUL, 0xDBBBC9D6UL, 0xACBCF940UL,
    0x32D86CE3UL, 0x45DF5C75UL, 0xDCD60DCFUL, 0xABD13D59UL,
    0x26D930ACUL, 0x51DE003AUL, 0xC8D75180UL, 0xBFD06116UL,
    0x21B4F4B5UL, 0x56B3C423UL, 0xCFBA9599UL, 0xB8BDA50FUL,
    0x2802B89EUL, 0x5F058808UL, 0xC60CD9B2UL, 0xB10BE924UL,
    0x2F6F7C87UL, 0x58684C11UL, 0xC1611DABUL, 0xB6662D3DUL,
    0x76DC4190UL, 0x01DB7106UL, 0x98D220BCUL, 0xEFD5102AUL,
    0x71B18589UL, 0x06B6B51FUL, 0x9FBFE4A5UL, 0xE8B8D433UL,
    0x7807C9A2UL, 0x0F00F934UL, 0x9609A88EUL, 0xE10E9818UL,
    0x7F6A0DBBUL, 0x086D3D2DUL, 0x91646C97UL, 0xE6635C01UL,
    0x6B6B51F4UL, 0x1C6C6162UL, 0x856530D8UL, 0xF262004EUL,
    0x6C0695EDUL, 0x1B01A57BUL, 0x8208F4C1UL, 0xF50FC457UL,
    0x65B0D9C6UL, 0x12B7E950UL, 0x8BBEB8EAUL, 0xFCB9887CUL,
    0x62DD1DDFUL, 0x15DA2D49UL, 0x8CD37CF3UL, 0xFBD44C65UL,
    0x4DB26158UL, 0x3AB551CEUL, 0xA3BC0074UL, 0xD4BB30E2UL,
    0x4ADFA541UL, 0x3DD895D7UL, 0xA4D1C46DUL, 0xD3D6F4FBUL,
    0x4369E96AUL, 0x346ED9FCUL, 0xAD678846UL, 0xDA60B8D0UL,
    0x44042D73UL, 0x33031DE5UL, 0xAA0A4C5FUL, 0xDD0D7CC9UL,
    0x5005713CUL, 0x270241AAUL, 0xBE0B1010UL, 0xC90C2086UL,
    0x5768B525UL, 0x206F85B3UL, 0xB966D409UL, 0xCE61E49FUL,
    0x5EDEF90EUL, 0x29D9C998UL, 0xB0D09822UL, 0xC7D7A8B4UL,
    0x59B33D17UL, 0x2EB40D81UL, 0xB7BD5C3BUL, 0xC0BA6CADUL,
    0xEDB88320UL, 0x9ABFB3B6UL, 0x03B6E20CUL, 0x74B1D29AUL,
    0xEAD54739UL, 0x9DD277AFUL, 0x04DB2615UL, 0x73DC1683UL,
    0xE3630B12UL, 0x94643B84UL, 0x0D6D6A3EUL, 0x7A6A5AA8UL,
    0xE40ECF0BUL, 0x9309FF9DUL, 0x0A00AE27UL, 0x7D079EB1UL,
    0xF00F9344UL, 0x8708A3D2UL, 0x1E01F268UL, 0x6906C2FEUL,
    0xF762575DUL, 0x806567CBUL, 0x196C3671UL, 0x6E6B06E7UL,
    0xFED41B76UL, 0x89D32BE0UL, 0x10DA7A5AUL, 0x67DD4ACCUL,
    0xF9B9DF6FUL, 0x8EBEEFF9UL, 0x17B7BE43UL, 0x60B08ED5UL,
    0xD6D6A3E8UL, 0xA1D1937EUL, 0x38D8C2C4UL, 0x4FDFF252UL,
    0xD1BB67F1UL, 0xA6BC5767UL, 0x3FB506DDUL, 0x48B2364BUL,
    0xD80D2BDAUL, 0xAF0A1B4CUL, 0x36034AF6UL, 0x41047A60UL,
    0xDF60EFC3UL, 0xA867DF55UL, 0x316E8EEFUL, 0x4669BE79UL,
    0xCB61B38CUL, 0xBC66831AUL, 0x256FD2A0UL, 0x5268E236UL,
    0xCC0C7795UL, 0xBB0B4703UL, 0x220216B9UL, 0x5505262FUL,
    0xC5BA3BBEUL, 0xB2BD0B28UL, 0x2BB45A92UL, 0x5CB36A04UL,
    0xC2D7FFA7UL, 0xB5D0CF31UL, 0x2CD99E8BUL, 0x5BDEAE1DUL,
    0x9B64C2B0UL, 0xEC63F226UL, 0x756AA39CUL, 0x026D930AUL,
    0x9C0906A9UL, 0xEB0E363FUL, 0x72076785UL, 0x05005713UL,
    0x95BF4A82UL, 0xE2B87A14UL, 0x7BB12BAEUL, 0x0CB61B38UL,
    0x92D28E9BUL, 0xE5D5BE0DUL, 0x7CDCEFB7UL, 0x0BDBDF21UL,
    0x86D3D2D4UL, 0xF1D4E242UL, 0x68DDB3F8UL, 0x1FDA836EUL,
    0x81BE16CDUL, 0xF6B9265BUL, 0x6FB077E1UL, 0x18B74777UL,
    0x88085AE6UL, 0xFF0F6A70UL, 0x66063BCAUL, 0x11010B5CUL,
    0x8F659EFFUL, 0xF862AE69UL, 0x616BFFD3UL, 0x166CCF45UL,
    0xA00AE278UL, 0xD70DD2EEUL, 0x4E048354UL, 0x3903B3C2UL,
    0xA7672661UL, 0xD06016F7UL, 0x4969474DUL, 0x3E6E77DBUL,
    0xAED16A4AUL, 0xD9D65ADCUL, 0x40DF0B66UL, 0x37D83BF0UL,
    0xA9BCAE53UL, 0xDEBB9EC5UL, 0x47B2CF7FUL, 0x30B5FFE9UL,
    0xBDBDF21CUL, 0xCABAC28AUL, 0x53B39330UL, 0x24B4A3A6UL,
    0xBAD03605UL, 0xCDD70693UL, 0x54DE5729UL, 0x23D967BFUL,
    0xB3667A2EUL, 0xC4614AB8UL, 0x5D681B02UL, 0x2A6F2B94UL,
    0xB40BBE37UL, 0xC30C8EA1UL, 0x5A05DF1BUL, 0x2D02EF8DUL,
};

typedef struct {
    uint32_t magic;//0xABCDDCBA
    uint32_t crc32;
    uint32_t patchsz;//patch package size
    uint32_t wrcnt;//timestamp
    uint32_t ctrlid;//state == BSDIFF_RESTORE0/1, progress of recovery
    uint32_t flitoff;//state == BSDIFF_FLIT, progress of flit
    uint8_t blockid;//manage block id
    uint8_t state;
    uint8_t is_confirm_addr;        /*支持固定back_up地址，无特殊需求，这里直接给0，有差分算法自动计算----0:差分算法自动按照从后向前计算地址，1:固定back_up/patch起始地址*/
    uint8_t reserved1[1];
    uint32_t flh_start_addr;        /*flash起始地址*/
    uint32_t flh_len;               /*flash总长度*/
    uint32_t backup_confirm_addr;   /*back_up起始地址固定，不执行从后向前计算*/
    uint32_t patch_confirm_addr;    /*patch起始地址固定，不执行从后向前计算*/
    uint8_t reserved2[20];
} manage_info_t;

typedef struct {
    uint32_t magic_ver;
    uint32_t bin_type;          /*SING-0x53494E47   DOUB-0x444F5542*/
    uint32_t src_crc32;         /*src bin crc32*/
    uint32_t dst_crc32;         /*dst bin crc32*/
    uint32_t src_length;        /*src bin length*/
    uint32_t dst_length;        /*dst bin length*/
    uint32_t bin_offset;        /*bin offset from FOTA Pkg Head*/
    uint32_t bin_start_addr;    /*bin start addr in ROM*/
    uint32_t bin_length;        /*bin length*/
    uint32_t bin_crc32;         /*bin crc32 checksum*/
    uint32_t step;              /*maxlen + step*/
    uint32_t buf_size;          /*buf_size*/
    uint8_t mode;               /*0:forward   1:backward*/
    uint8_t reserved1[1];
    uint16_t ver_supp_min_code; /*ver_supp_min_code*/
    uint8_t reserved2[12];
}diff2ya_header_t;

static unsigned int tuya_hash_crc32i_update (unsigned int hash,const void *data,unsigned int size)
{
    for (; size != 0; size--)
    {
        hash = s_crc32[(hash ^ *(unsigned char*)data) & 0xFF] ^ (hash >> 8);
        data = (const unsigned char*)data + 1;
    }
    return ((unsigned int)(hash));
}

static unsigned int tuya_hash_crc32i_finish (unsigned int hash)
{
    return ((unsigned int)(hash ^ CRC32_XOR_VALUE));
}

static uint32_t  tuya_flash_crc32_cal(uint32_t addr, uint32_t size)
{
    uint32_t read_block = BLOCK_SIZE;
    uint8_t * read_buffer = (uint8_t *)os_calloc(1, BLOCK_SIZE);
    
    if(read_buffer == NULL) 
    {
        os_printf(LM_APP, LL_INFO, "malloc error \n");
        return -1;
    }

    uint32_t read_len = 0;
    uint32_t pos = 0;
    
    uint32_t crc32 = CRC32_INT_VALUE;
    while (1) 
    {
        read_len = size - pos > read_block ? read_block : size - pos;
        int ret = tkl_flash_read(addr + pos, read_buffer, read_len);
        if(ret) 
        {
            os_free(read_buffer);
            return ret;
        }
        crc32 = tuya_hash_crc32i_update(crc32, read_buffer, read_len);
        pos += read_len;
        if (pos >= size) 
        {
            break;
        }
    }
    crc32 = tuya_hash_crc32i_finish(crc32);
    
    os_free(read_buffer);
    return crc32;
}

static int path_bin_crc()
{
    uint32_t crc_path_bin = 0;
    crc_path_bin = tuya_flash_crc32_cal(read_path_start_addr + sizeof(diff2ya_header_t), patch_size - sizeof(diff2ya_header_t));
    
    os_printf(LM_APP, LL_INFO, "path_bin_crc_new = 0x%x , crc_path_bin = 0x%x \n", path_bin_crc_new, crc_path_bin);
    if (path_bin_crc_new != crc_path_bin)
    {
        os_printf(LM_APP, LL_ERR, " ############path bin crc error##############\n");
        return -1;
    }

    return 0;
}

#if 0
static uint32_t _cal_crc32(uint8_t *buf, uint32_t len)
{
    uint32_t i, crc32= 0;
    for(i = 0; i < len; i++) 
    {
      crc32 += buf[i];
    }

    return crc32;
}
#endif

static int check_path_head(unsigned char* data, unsigned int total_len)
{
    unsigned int flash_max_len = 0;
    //unsigned int path_head_max_len = 0;
    unsigned int flash_start_addr_head = 0;
    unsigned int flash_start_len_head = 0;
    
    diff2ya_header_t head_path1 = {0};
    diff2ya_header_t* head_path = &head_path1;
    memcpy(head_path, data, sizeof(diff2ya_header_t));
    partion_info_get(PARTITION_NAME_CPU, &flash_start_addr_head, &flash_start_len_head);

    //获取flash长度
    flash_max_len = flash_start_len_head + 2 * 4096;
    
    //判断添加path.bin后flash是否会超出范围
    uint32_t firm_pack_len = (head_path->dst_length > head_path->src_length) ? head_path->dst_length : head_path->src_length;
    if (firm_pack_len%4096) {
        firm_pack_len += 4096 - firm_pack_len%4096;
    }
    uint32_t max_len = firm_pack_len + head_path->bin_length + head_path->step + head_path->buf_size + 2*4096;
    if (max_len%4096) {
        max_len += 4096 - max_len%4096;
    }
    
    os_printf(LM_APP, LL_INFO, "path_head_max_len = %d , flash_max_len = %d magic_ver = 0x%x \n", max_len, flash_max_len, head_path->magic_ver);
    if(flash_max_len < max_len) {
        os_printf(LM_APP, LL_ERR, "path bin size more max! \n");
        goto EXIT;
    } else {
        os_printf(LM_APP, LL_INFO,"max len...firm_pack_len:0x%x, max_len:0x%x\r\n", firm_pack_len, max_len);
    }

    
    //校验path包头标记为是否为差分包
    if ((head_path->magic_ver != TUYA_OTA_PATH) || (head_path->bin_type != 0x53494E47))
    {
        os_printf(LM_APP, LL_ERR, "path bin head magic_ver is error! \n");
        goto EXIT;
    }
    
    //获取包头中差分包crc校验值，用于拉完包后做校验对比
    path_bin_crc_new = head_path->bin_crc32;
    return 0;

EXIT:
    return -1; 
}

/**
* @brief OTA差分升级时写标记位
* param[in]        size       patch大小
*/

static void ota_set_info(unsigned int size)
{
    unsigned int mag_flash_start_addr = 0;
    unsigned int mag_flash_len = 0;
    unsigned int cpu_flash_start_addr = 0;
    unsigned int cpu_flash_len = 0;


    //获取flash起始地址及长度
    partion_info_get(PARTITION_NAME_CPU, &cpu_flash_start_addr, &cpu_flash_len);
    partion_info_get(PARTITION_NAME_OTA_STATUS, &mag_flash_start_addr, &mag_flash_len);
    
    manage_info_t mag = {0};
    mag.magic = 0XABCDDCBA;
    mag.patchsz = size;
    mag.blockid = 0;
    mag.state = 1;
    mag.flh_start_addr = cpu_flash_start_addr;
    mag.flh_len = cpu_flash_len + mag_flash_len;
    mag.patch_confirm_addr = 0;

    unsigned int i = 0, crc32 = 0;
    unsigned char *buf = (unsigned char*)&mag;
    for(i = 8; i < sizeof(manage_info_t); i++)
    {
        crc32 += buf[i];
    }
    mag.crc32 = crc32;
    os_printf(LM_APP, LL_INFO,"cpu_flash_start_addr = 0x%x , cpu_flash_len = %d ,mag_flash_start_addr = %d 0x%x \n", cpu_flash_start_addr, cpu_flash_len, mag_flash_start_addr, mag_flash_start_addr);
    tkl_flash_erase(mag_flash_start_addr, 4096 * 2);
    tkl_flash_write(mag_flash_start_addr, (unsigned char *)&mag, sizeof(manage_info_t));
}

static int tuya_ota_done(bool reset)
{
/*
    int i = 0;
    unsigned char * read_buff = (unsigned char *)os_calloc(1, 1024);
    tkl_flash_read(read_addr, read_buff, 1024);
    os_printf(LM_APP, LL_INFO, "read_buff: \n", * (read_buff + i));
    for (i = 0; i < 1024; i++)
    {
        os_printf(LM_APP, LL_INFO, "%x  ", * (read_buff + i));
        if ((i != 0) && ((i % 40) == 0))
        {   
            os_printf(LM_APP, LL_INFO, "\n");
        }
    }
    os_free(read_buff);
*/

    if (reset) 
    {
        os_printf(LM_APP, LL_INFO, "ota done successs, reset now\n\n");
        tkl_system_reset();
    }

    return 0;
}


static int tuya_ota_flash_space_erase(unsigned int addr, unsigned int len, unsigned int flash_start_addr, unsigned int flash_len)
{
    unsigned int start_erase_num = 0;
    unsigned int erase_start_addr = 0;
    unsigned int eraselen = 0;
    unsigned int flash_erase_end_addr = 0;
    unsigned int sector_size = 4096;

    int erase_len_used = 0;
    int erase_len_unusd = 0;

    int buf_len = 0;
    
    if (0 == addr || 0 == len || 0 == flash_start_addr || 0 == flash_len) 
    {
        return -1;
    }

    //首先判断地址是否是4k对其
    if ((addr % sector_size) == 0)
    {
        //如果地址是4k对其，则当前地址就是开始擦除地址
        erase_start_addr = addr;
        start_erase_num = len / sector_size;
        //如果数据长度不满一个扇区4k或者长度不是4K(扇区)对其则删除其所在的一个扇区
        if((0 == start_erase_num) || (len % sector_size != 0)) {
            start_erase_num++;
        }
    }
    else
    {
        //计算扇区内已被已用空间大小
        erase_len_used = addr % sector_size;
        //计算在扇区中剩余空间大小
        erase_len_unusd = sector_size - erase_len_used;
        //判断len是否在扇区剩余空间内
        buf_len = len;

        if ((buf_len - erase_len_unusd) <= 0)
        {
            //如果len在剩余空间内则不用擦除
            //os_printf(LM_APP, LL_INFO, "no erase, flash is erase! \n");
            return 0;
        }
        else
        {
            //如果len超出剩余空间则继续擦除
            start_erase_num = (len - erase_len_unusd) / sector_size + 1;
            erase_start_addr = (addr + 4096) / 4096 * 4096;             
        }
    }
    //算出需要擦除几个扇区
    eraselen = start_erase_num * sector_size;

    flash_erase_end_addr = flash_start_addr + flash_len;
    
    if ((erase_start_addr + eraselen) > flash_erase_end_addr)
    {
        os_printf(LM_APP, LL_ERR, " err: erase len > erase end len!  erase_start_addr[%d]  eraselen[%d] flash_erase_end_addr[%d]\n", erase_start_addr, eraselen, flash_erase_end_addr);
        return -1;
    }
    //os_printf(LM_APP, LL_INFO, "erase_start_addr = 0x%x, eraselen = %d \n", erase_start_addr, eraselen);

    return tkl_flash_erase(erase_start_addr, eraselen);
}

static int tuya_ota_write(unsigned char *data, unsigned int len, unsigned int total_len)
{
    unsigned int flash_start_addr_img = 0;
    unsigned int flash_start_len_img = 0;
    unsigned int start_image_addr = 0;
    int ret = 0;
    
    partion_info_get(PARTITION_NAME_CPU, &flash_start_addr_img, &flash_start_len_img);
    //os_printf(LM_APP, LL_INFO, " flash_start_addr = 0x%x %d flash_start_len = 0x%x %d \n", flash_start_addr_img, flash_start_addr_img, flash_start_len_img, flash_start_len_img);

    start_image_addr = (flash_start_addr_img + flash_start_len_img - total_len) / 4096 * 4096;
    read_path_start_addr = start_image_addr;
    
    
    ret = tuya_ota_flash_space_erase(start_image_addr + write_flash_data_len, len, flash_start_addr_img, flash_start_len_img);
    
    if ( ret != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:flash erase interface error ret = %d \n" , ret);
        return -1;
    }
    
    if (tkl_flash_write(start_image_addr + write_flash_data_len, data, len) != 0)
    {
        os_printf(LM_APP, LL_ERR, "OTA:flash write interface error");
        return -1;
    }

    write_flash_data_len += len;
    int new_progress = (int) write_flash_data_len * 100 / total_len;
    if ((download_progress+4) <= new_progress)
    {
        download_progress = new_progress;
        os_printf(LM_APP, LL_INFO, "OTA:ota download firmware %d%%\n", download_progress);
    }

    return 0;
}

static void tuya_ota_flg_reset()
{
    start_ota_data = 0;
    write_flash_data_len = 0;
    download_progress = 0;
}
#endif
/**
 * @brief 升级开始通知函数
 * 
 * @param[in] file_size 升级固件大小
 * 
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_start_notify(UINT_T image_size, TUYA_OTA_TYPE_E type, TUYA_OTA_PATH_E path)
{
    if(image_size == 0) 
    {
        return OPRT_INVALID_PARM;
    }
#if TUYA_OTA_UPGRADE
    tuya_ota_flg_reset();
#else
    if (ota_init() != 0) {
        return OPRT_COM_ERROR;
    }
#endif
    
    os_printf(LM_APP, LL_INFO, "*******************start OTA******************* \n");
    return OPRT_OK;
}

/**
 * @brief ota数据包处理
 * 
 * @param[in] total_len ota升级包总大小
 * @param[in] offset 当前data在升级包中的偏移
 * @param[in] data ota数据buffer指针
 * @param[in] len ota数据buffer长度
 * @param[out] remain_len 内部已经下发但该函数还未处理的数据长度
 * @param[in] pri_data 保留参数
 *
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_data_process(TUYA_OTA_DATA_T *pack, UINT_T* remain_len)
{
#if TUYA_OTA_UPGRADE
	patch_size = pack->total_len;
if (start_ota_data == 0)
    {
        start_ota_data = 1;
        if (check_path_head(pack->data, pack->total_len) != 0)
        {
            tuya_ota_flg_reset();
            os_printf(LM_APP, LL_ERR, "check path bin head error! \n");
            return OPRT_COM_ERROR;
        }
    }
    
    if (tuya_ota_write(pack->data, pack->len, pack->total_len) != 0) {
        tuya_ota_flg_reset();
        os_printf(LM_APP, LL_ERR, "OTA WRITE FLASH ERROR! \n");
        return OPRT_COM_ERROR;
    }
#else
    if (ota_write(data, len) != 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
#endif
    return OPRT_OK;
}

/**
 * @brief 固件ota数据传输完毕通知
 *        用户可以做固件校验以及设备重启
 * param[in]        reset       是否需要重启
 * @retval  =0      成功
 * @retval  <0      错误码
 */
OPERATE_RET tkl_ota_end_notify(BOOL_T reset)
{
#if TUYA_OTA_UPGRADE
    //合入涂鸦ota差分代码，在下载完固件重启之前在manage中写入标记位
    if (path_bin_crc())
    {
        tuya_ota_flg_reset();
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    
    if (patch_size != 0)
    {
        ota_set_info(patch_size);
    }
    
    if (tuya_ota_done(reset) != 0) {
        tuya_ota_flg_reset();
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
#else
    if (ota_done(reset) != 0) {
        return OPRT_COM_ERROR;
    }
#endif

    return OPRT_OK;
}


/**
* @brief get ota ability
*
* @param[out] image_size:  max image size
* @param[out] type:        ota type
*
* @note This API is used for get chip ota ability
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_ota_get_ability(UINT_T *image_size, UINT32_T *type)
{
    *image_size = 0xFFFFFFFF;
    *type = TUYA_OTA_DIFF;

    return OPRT_OK;
}
