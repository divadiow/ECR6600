#ifndef __OTA_H__
#define __OTA_H__


#include "lwip/sockets.h"


#define OTA_STAT_PART_SIZE      (4 * 1024)
#define OTA_BACKUP_PART_SIZE    (28 * 1024)


#define OTA_UPDATE_METHOD_CZ    0x22
#define OTA_UPDATE_METHOD_DI    0x42
#define OTA_UPDATE_METHOD_AB    0x82

#define OTA_SIZE_ALIGN_4K(x)    ((x + 0xFFF) & (~0xFFF))

#define OTA_DEFAULT_BUFF_LEN    256
#define OTA_VERSION_NUM         6

#define OTA_AB_UPDATAFLAG       (0x80)
#define OTA_MOVE_CZ_A2B         (0x40)
#define OTA_AB_SELECT_B         (0x10)
#define OTA_AB_DIRETC_START     (0x08)
#define OTA_AB_TRY_COUNT        (0x07)

#define DOWNLOAD_OTA_PARTA      0x1
#define DOWNLOAD_OTA_PARTB      0x2

typedef enum
{
	OTA_UPDATE_NONE,
	OTA_UPDATE_SUCCESS,
	OTA_UPDATE_FAIL,
	OTA_UPDATE_MAX
} ota_update_status;


typedef void (*ota_cb)(void);
typedef struct
{
	ota_cb ota_success_cb;
	ota_cb ota_fail_cb;
	ota_cb ota_action_cb;
} ota_cb_t;


typedef enum
{
    OTA_ERROR_NONE,                 // ota 没有错误
    OTA_ERROR_INIT_FAIL,            // ota hal 初始化错误
    OTA_ERROR_WRITE_FAIL,           // ota hal 写入错误
    OTA_ERROR_HTTP_START_FAIL,      // ota http 启动错误
    OTA_ERROR_SOCKET_SEND_FAIL,     // ota 套接口发送错误
    OTA_ERROR_SOCKET_RECEIVE_FAIL,
    OTA_ERR_MAX
} ota_error_status;

typedef enum
{
    OTA_DOWNLOAD_IDLE,              // 空闲
    OTA_DOWNLOAD_ING,               // 下载中
    OTA_DOWNLOAD_COMPLETE           // 下载完成
} ota_download_status;


typedef enum
{
	OTA_PACKET_BROADCAST = 0x01,
	OTA_PACKET_REPORT    = 0x10,
	OTA_PACKET_ACTION    = 0x11,
} ota_packet_type;

typedef enum
{
	OTA_ACTION_SUCESS,
	OTA_ACTION_UPDATE,
	OTA_ACTION_ILLEGAL,
	OTA_ACTION_FAIL,
} ota_action;

typedef struct
{
	unsigned char type;
	unsigned char len;
	char url[OTA_DEFAULT_BUFF_LEN];
	unsigned char srcver[OTA_VERSION_NUM];
	unsigned char destver[OTA_VERSION_NUM];
	unsigned int fm_len;
	unsigned int fm_crc;
} ota_recv_pack_t;

typedef struct
{
	unsigned char type;
	unsigned char mac[6];
	unsigned char ip[4];
	unsigned char version[OTA_VERSION_NUM];
	unsigned char action;
	unsigned char crc;
} ota_send_pack_t;

typedef struct
{
	unsigned char version[OTA_VERSION_NUM];
	char path[OTA_DEFAULT_BUFF_LEN];
	unsigned char src_ip[4];
	char dst_ip[16];
	unsigned char mac[6];
	struct sockaddr_in client_addr;
} ota_info_t;

typedef struct {
    unsigned char magic[3];
    unsigned char len;
    unsigned int  crc;
    unsigned int  patch_addr;
    unsigned int  patch_size;
    unsigned char update_flag;
    unsigned char update_state;
    unsigned char updata_ab;
    unsigned char reserved;
    unsigned int ctrl;
    unsigned int diff;
    unsigned int exd;
    unsigned int startpos;
    unsigned int movecnt;
    unsigned int movenum;
} ota_state_t;

typedef struct {
    unsigned char magic[7];
    unsigned char version;
    unsigned int package_size;
    unsigned int package_crc;
    unsigned int boot_size;
    unsigned int firmware_size;
    unsigned int firmware_new_size;
    unsigned int firmware_crc;
    unsigned int firmware_new_crc;
    unsigned char reserved[12];
    unsigned char source_version[32];
    unsigned char target_version[32];
} ota_package_head_t;

typedef struct {
	unsigned int	image_magic;
	unsigned int	header_version;			/* image_header struct version V0 */
	unsigned int	image_ep;				/* Entry Point Address */
	unsigned int	text_load_addr;			/* download addr */
	unsigned int	text_size;				/* Image text Size */
	unsigned int	data_load_addr;			/* download addr */
	unsigned int	data_size;				/* Image Data Size */
	unsigned int	xip_size;				/* Image xip Size */
	unsigned char	signature[64];			/* ecdsa  (r,s) */
	unsigned char	release_version[12];	/* release_version */
	unsigned int	image_dcrc;				/* Image Data CRC Checksum        */
	unsigned int	update_method;			/* ota data, for details, see @ota_control */
	unsigned char	reserved[136];
	unsigned int	image_hcrc;				/* Image Head CRC Checksum        */
} image_headinfo_t;



int ota_init(void);
int ota_write(unsigned char *data, unsigned int len);
int ota_done(bool reset);
int ota_confirm_update(void);
int ota_get_flash_crc(unsigned int addr, unsigned int size);
int ota_firmware_check(unsigned int data_len, unsigned int crc);
#ifdef CONFIG_SPI_SERVICE
int ota_update_image(unsigned char *data, unsigned int len);
#endif
#endif
