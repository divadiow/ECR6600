/*******************************************************************************
 * Copyright by Transa Semi.
 *
 * File Name: amtNV.h   
 * File Mark:    
 * Description:  
 * Others:        
 * Version:       v0.1
 * Author:        liangyu
 * Date:          2020-2-24
 * History 1: 
 *     Date: 2020-04-17
 *     Version:
 *     Author: wangxia
 *     Modification:  add aliyun info in AMT partition

 * History 2: 
  ********************************************************************************/

#ifndef AMTNV_H
#define AMTNV_H

//#include "nv_config.h"


/****************************************************************************
* 	                                        Include files
****************************************************************************/


/****************************************************************************
* 	                                        Macros
****************************************************************************/
/**************************************************************************** 
*
*							AMT  NV  PARTION
*
*	AMT_NV_XXX			--- the address of the NV parameter
*	AMT_NV_XXX_len		--- the length of the NV parameter
*	AMT_NV_XXX_OFFSET	--- the offset length from the next NV parameter
*	
****************************************************************************/


#define  MAC_OTP_ADDR_GD25Q80E_FLASH 		 0x1030



extern unsigned int amt_partion_base;
#define AMT_PARTION_SIZE					4096

#define AMT_PARTION_END						(amt_partion_base + AMT_PARTION_SIZE)

#define AMT_NV_SN							amt_partion_base
#define AMT_NV_SN_LEN						64
#define AMT_NV_SN_OFFSET					72
	
#define AMT_NV_MAC							(AMT_NV_SN + AMT_NV_SN_OFFSET)
#define AMT_NV_MAC_LEN						18
#define AMT_NV_MAC_OFFSET					24
	
#define AMT_NV_PIN_FLAG						(AMT_NV_MAC + AMT_NV_MAC_OFFSET)
#define AMT_NV_PIN_FLAG_LEN					2
#define AMT_NV_PIN_FLAG_OFFSET				8
	
#define AMT_NV_PSM_FLAG						(AMT_NV_PIN_FLAG + AMT_NV_PIN_FLAG_OFFSET)
#define AMT_NV_PSM_FLAG_LEN					2
#define AMT_NV_PSM_FLAG_OFFSET				8
	
#define AMT_NV_WIFI_FLAG					(AMT_NV_PSM_FLAG + AMT_NV_PSM_FLAG_OFFSET)
#define AMT_NV_WIFI_FLAG_LEN				2
#define AMT_NV_WIFI_FLAG_OFFSET				8
	
#define AMT_NV_UPDATE_FLAG					(AMT_NV_WIFI_FLAG + AMT_NV_WIFI_FLAG_OFFSET)
#define AMT_NV_UPDATE_FLAG_LEN				2
#define AMT_NV_UPDATE_FLAG_OFFSET			8
	
#define AMT_NV_SN_FLAG						(AMT_NV_UPDATE_FLAG + AMT_NV_UPDATE_FLAG_OFFSET)
#define AMT_NV_SN_FLAG_LEN					2
#define AMT_NV_SN_FLAG_OFFSET				8
	
#define AMT_NV_MAC_FLAG						(AMT_NV_SN_FLAG + AMT_NV_SN_FLAG_OFFSET)
#define AMT_NV_MAC_FLAG_LEN					2
#define AMT_NV_MAC_FLAG_OFFSET				8
	
#define AMT_NV_FREQOFFSET_FLAG				(AMT_NV_MAC_FLAG + AMT_NV_MAC_FLAG_OFFSET)
#define AMT_NV_FREQOFFSET_FLAG_LEN			2
#define AMT_NV_FREQOFFSET_FLAG_OFFSET		8
	
#define AMT_NV_FREQOFFSET					(AMT_NV_FREQOFFSET_FLAG + AMT_NV_FREQOFFSET_FLAG_OFFSET)
#define AMT_NV_FREQOFFSET_LEN				8
#define AMT_NV_FREQOFFSET_OFFSET			16
	
#define AMT_NV_TXPOWER_FLAG					(AMT_NV_FREQOFFSET + AMT_NV_FREQOFFSET_OFFSET)
#define AMT_NV_TXPOWER_FLAG_LEN				2
#define AMT_NV_TXPOWER_FLAG_OFFSET			8
	
#define AMT_NV_TXPOWER_HTCH1				(AMT_NV_TXPOWER_FLAG + AMT_NV_TXPOWER_FLAG_OFFSET)
#define AMT_NV_TXPOWER_HTCH1_LEN			8
#define AMT_NV_TXPOWER_HTCH1_OFFSET			16
	
#define AMT_NV_TXPOWER_HTCH6				(AMT_NV_TXPOWER_HTCH1 + AMT_NV_TXPOWER_HTCH1_OFFSET)
#define AMT_NV_TXPOWER_HTCH6_LEN			8
#define AMT_NV_TXPOWER_HTCH6_OFFSET			16
	
#define AMT_NV_TXPOWER_HTCH11				(AMT_NV_TXPOWER_HTCH6 + AMT_NV_TXPOWER_HTCH6_OFFSET)
#define AMT_NV_TXPOWER_HTCH11_LEN			8
#define AMT_NV_TXPOWER_HTCH11_OFFSET		16
	
#define AMT_NV_TXPOWER_HTCH3				(AMT_NV_TXPOWER_HTCH11 + AMT_NV_TXPOWER_HTCH11_OFFSET)
#define AMT_NV_TXPOWER_HTCH3_LEN			8
#define AMT_NV_TXPOWER_HTCH3_OFFSET			16
	
#define AMT_NV_TXPOWER_HTCH8				(AMT_NV_TXPOWER_HTCH3 + AMT_NV_TXPOWER_HTCH3_OFFSET)
#define AMT_NV_TXPOWER_HTCH8_LEN			8
#define AMT_NV_TXPOWER_HTCH8_OFFSET			16
	
#define AMT_NV_TXPOWER_NONHTCH1				(AMT_NV_TXPOWER_HTCH8 + AMT_NV_TXPOWER_HTCH8_OFFSET)
#define AMT_NV_TXPOWER_NONHTCH1_LEN			8
#define AMT_NV_TXPOWER_NONHTCH1_OFFSET		16
	
#define AMT_NV_TXPOWER_NONHTCH6				(AMT_NV_TXPOWER_NONHTCH1 + AMT_NV_TXPOWER_NONHTCH1_OFFSET)
#define AMT_NV_TXPOWER_NONHTCH6_LEN			8
#define AMT_NV_TXPOWER_NONHTCH6_OFFSET		16
	
#define AMT_NV_TXPOWER_NONHTCH11			(AMT_NV_TXPOWER_NONHTCH6 + AMT_NV_TXPOWER_NONHTCH6_OFFSET)
#define AMT_NV_TXPOWER_NONHTCH11_LEN		8
#define AMT_NV_TXPOWER_NONHTCH11_OFFSET		16
	
#define AMT_NV_TXPOWER_11BCH1				(AMT_NV_TXPOWER_NONHTCH11 + AMT_NV_TXPOWER_NONHTCH11_OFFSET)
#define AMT_NV_TXPOWER_11BCH1_LEN			8
#define AMT_NV_TXPOWER_11BCH1_OFFSET		16
	
#define AMT_NV_TXPOWER_11BCH6				(AMT_NV_TXPOWER_11BCH1 + AMT_NV_TXPOWER_11BCH1_OFFSET)
#define AMT_NV_TXPOWER_11BCH6_LEN			8
#define AMT_NV_TXPOWER_11BCH6_OFFSET		16
	
#define AMT_NV_TXPOWER_11BCH11				(AMT_NV_TXPOWER_11BCH6 + AMT_NV_TXPOWER_11BCH6_OFFSET)
#define AMT_NV_TXPOWER_11BCH11_LEN			8
#define AMT_NV_TXPOWER_11BCH11_OFFSET		16
	
#define AMT_NV_CALIBRATION					(AMT_NV_TXPOWER_11BCH11 + AMT_NV_TXPOWER_11BCH11_OFFSET)
#define AMT_NV_CALIBRATION_LEN				2
#define AMT_NV_CALIBRATION_OFFSET			8

#define AMT_NV_ALIYUN_PDKEY					(AMT_NV_CALIBRATION + AMT_NV_CALIBRATION_OFFSET)
#define AMT_NV_ALIYUN_PDKEY_LEN				32
#define AMT_NV_ALIYUN_PDKEY_OFFSET			40

#define AMT_NV_ALIYUN_PDSECRET				(AMT_NV_ALIYUN_PDKEY + AMT_NV_ALIYUN_PDKEY_OFFSET)
#define AMT_NV_ALIYUN_PDSECRET_LEN			32
#define AMT_NV_ALIYUN_PDSECRET_OFFSET		40

#define AMT_NV_ALIYUN_DEVNAME				(AMT_NV_ALIYUN_PDSECRET + AMT_NV_ALIYUN_PDSECRET_OFFSET)
#define AMT_NV_ALIYUN_DEVNAME_LEN			32
#define AMT_NV_ALIYUN_DEVNAME_OFFSET		40

#define AMT_NV_ALIYUN_DEVSECRET				(AMT_NV_ALIYUN_DEVNAME + AMT_NV_ALIYUN_DEVNAME_OFFSET)
#define AMT_NV_ALIYUN_DEVSECRET_LEN			32
#define AMT_NV_ALIYUN_DEVSECRET_OFFSET		40

#define AMT_NV_ALIYUN_FLAG					(AMT_NV_ALIYUN_DEVSECRET + AMT_NV_ALIYUN_DEVSECRET_OFFSET)
#define AMT_NV_ALIYUN_FLAG_LEN				2
#define AMT_NV_ALIYUN_FLAG_OFFSET			8

#define AMT_NV_TEST_FLAG					(AMT_NV_ALIYUN_FLAG + AMT_NV_ALIYUN_FLAG_OFFSET)
#define AMT_NV_TEST_FLAG_LEN				10
#define AMT_NV_TEST_FLAG_OFFSET				16
	
// continuous updating .....
	
#define AMT_NV_MAX							(amt_partion_base + 2048 -1)//(AMT_NV_MAC_FLAG + AMT_NV_MAC_FLAG_OFFSET)





typedef enum
{
		/* 0 ~ -29: commom */
		DRV_SUCCESS 									= 0,		/* successed */
		DRV_ERROR										 = -1,		/* failed */
		DRV_ERR_INVALID_IOCTL_CMD = -2, 	 /* no this control command branch */
		DRV_ERR_NOT_SUPPORTED		   = -3,	  /* this function hasn't been supported */
		DRV_ERR_INVALID_PARAM		   = -4,	  /* the input parameter is invalid */
		DRV_ERR_MEM_ALLOC				   = -5,	  /* failed to malloc memory */
		DRV_ERR_HISR_CREATE_FAIL	 = -6,		/* failed to create hisr */
		DRV_ERR_TIMEOUT 						= -7,	   /* timeout for a block waitting operation */
		DRV_ERR_BUSY								 = -8,		/* busy now to do the request operation */
		DRV_ERR_NOT_OPENED				   = -9,	  /* the device to operate hasn't been opened yet */
		DRV_ERR_OPEN_TIMES					= -10,	  /* try to open a device which has been opened already */
		DRV_ERR_NOT_STARTED 			  = -11,	/* the device to operate hasn't been started yet */
		DRV_ERR_START_TIMES 			   = -12,	 /* try to open a device which has been opened already */
		/* reserved */
	
		/* -30 ~ -39: for dal */
		DRV_ERR_DEV_OVERFLOW	 = -30, 	  /* no free entry to install this device. please change ZDRV_MAX_DEV_NUM in dal_api.h */
		DRV_ERR_DEV_TABLE				= -31,		 /* the device table has been destroyed */
		DRV_ERR_FD_OVERFLOW 	   = -32,		/* no free entry to open this device. pleas change ZDRV_MAX_DEV_FILE_NUM in dal_api.h */
		DRV_ERR_FD_TABLE				  = -33,	  /* the file descriptor table has been destroyed */
		DRV_ERR_INSTALLED_TIMES = -34,		/* try to install a device which hasn been installed yet */
		DRV_ERR_NO_THIS_DEVICE	  = -35,	  /* try to open a device which hasn't been installed yet */
		/* reserved */
		
		/*-40 ~ -59: for sio */
		DRV_ERR_NO_CHANNEL					 = -40, 		/*the used sio no channel*/
		DRV_ERR_CHAN_CREATE_FAIL	  = -41,		   /*the  sio creat channel fail*/
		DRV_ERR_DEV_STATE_WRONG 	 = -42, 		 /*the	sio state error*/
		DRV_ERR_CHAN_DELETE_FAIL	  = -43,		  /*the  sio delete channel fail*/
		DRV_ERR_DEV_READ						 = -44, 	 /*the	sio read data error*/
		DRV_ERR_CHAN_SEM_USED			 = -45, 		/*the  sio semp has been used r*/
		DRV_ERR_CHAN_DELETED			   = -46,		/*the  sio channel has been deleted */
		DRV_ERR_DEV_CLOSED					  = -47,		/*the  sio has been closed */
		DRV_ERR_DEV_OPT_NULL				= -48,		 /*the	sio device ptr is null*/
		DRV_ERR_INSTALL_DRIVER_FAIL = -49,			/*the  sio install	faill*/
		DRV_ERR_BUFFER_NOT_ENOUGH = -50,			/*the  sio data buffer not enough*/
		/* reserved */
	
		
		/* -60 ~ 69: for mux */
		DRV_ERR_MUX_INVALID_DLCI		  = -60,	  /* the dlci is invalid */
		DRV_ERR_MUX_BUSY							 = -61, 	 /* busy now, so the required operation has been rejected */
		DRV_ERR_MUX_NOT_READY				= -62,		/* the mux or dlci is not ready to do this required operation */
		DRV_ERR_MUX_FLOW_CONTROLED = -63,	  /* this dlc is flow-controled by the opposite station, so can't sent data any more */
		DRV_ERR_MUX_PN_REJECTED 		   = -64,	  /* the parameter for this dlc establishment is rejected by the opposite station */
		DRV_ERR_MUX_BUF_IS_FULL 			 = -65, 	 /* the data buffer of this dlc is full, so can't write any more */
		DRV_ERR_MUX_BUF_IS_EMPTY		  = -66,	  /* the data buffer of this dlc is empty, so no data to transfer */
		DRV_ERR_MUX_FRAME_INVALID	   = -67,	   /* the frame data is invalid */
		DRV_ERR_MUX_FRAME_UNCOMPLETE	= -68,		/* the frame data is uncomplete */
	
		DRV_ERROR_EMPTY = -90,
		DRV_ERROR_FULL = -91,
		DRV_ERROR_NODEV = -92,
		DRV_ERROR_SUSPEND = -93,
		DRV_ERROR_AGAIN = -94,
		DRV_ERROR_ABORT = -95,
		DRV_ERROR_NOCONNECT = -96,
	 
		/*-100~-104 for spi*/
		DRV_ERR_NOCOMPLETE				= -100,
	
		/*-105~-109 for gpio*/
		DRV_ERR_NOT_WRITE				= -105,
	
		/*-110~-119 for pmic */ 	 
		DRV_ERR_CLIENT_NBOVERFLOW		= -110, /*!< The requested operation could not becompleted because there are too many PMIC client requests */
		DRV_ERR_SPI_READ				= -111, 		
		DRV_ERR_SPI_WRITE				= -112,    
		DRV_ERR_EVENT_NOT_SUBSCRIBED	= -113, /*!< Event occur and not subscribed 	  */
		DRV_ERR_EVENT_CALL_BACK 		= -114, /*!< Error - bad call back				  */   
		DRV_ERR_UNSUBSCRIBE 			= -115, /*!< Error in un-subscribe event		  */
	
		/*-120~-129 for sd */ 
		DRV_ERR_INTR_TIMEOUT			= -120,
		DRV_ERR_INTR_ERROR				= -121,
		DRV_ERR_CARDSTATE_ERROR 		= -122,
		DRV_ERR_CARD_DISCONNECT 		= -123,
		DRV_ERR_WRITE_PROTECT			= -124,
		DRV_ERR_PWD_ERR 				= -125,
		DRV_ERR_LOCKCARD_ERR			= -126,
		DRV_ERR_FORCEERASE_ERR			= -127,
		DRV_ERR_RESPONSE_ERR			= -128,
		DRV_ERR_HLE_ERROR			   = -129,
		DRV_ERR_EIO 					= -130, /*IO Error*/
		DRV_ERR_ERANGE					= -131, /* Math result not representable */
		DRV_ERR_EINPROGRESS 			= -132, /* Operation now in progress */
		DRV_ERR_ENODEV					= -133, /*no such device*/
		DRV_ERR_BADMSG					= -134, /*not a date message*/
		DRV_ERR_ENOENT					= -135, /* No such file or directory */ 
		DRV_ERR_ILSEQ					= -136, /* Illegal byte sequence */
	
		/*-137~-140 for i2c */ 
		DRV_ERR_ADDR_TRANSFER			= -137,
		DRV_ERR_DATA_TRANSFER			= -138,
		DRV_ERR_AGAIN					= -139,
		DRV_ERR_NOACK					= -140,
	
	
	
		/*-141~-156 for usb */ 
		DRV_ERR_NOT_READY					= -141,
		DRV_ERR_STATUS_BUSY 			= -142,
		DRV_ERR_STALL						= -143,
		DRV_ERR_END 							= -144,
		DRV_ERR_USB_BUF_STATE		= -145,
		DRV_ERR_USB_BUF_FULL			= -146,
		DRV_ERR_USB_BUF_EMPTY		  = -147,
		DRV_ERR_USB_QMI_UNDERRUN  = -148,
		DRV_ERR_USB_QMI_OVERRUN 	= -149,
		DRV_ERR_USB_QMI_READ0		  = -150,
		DRV_ERR_USB_UNCONNECTED    = -151,
		DRV_ERR_QMI_HEADER_ERR		 = -152,
		DRV_ERR_QMI_CTL_ERR 	  = -153,
		DRV_ERR_QMI_WDS_ERR 	  = -154,
		DRV_ERR_QMI_DMS_ERR 	  = -155,
		DRV_ERR_QMI_NAS_ERR 	  = -156,
	
		/*-160~-180 for nand */
		DRV_ERR_LEN_ADDRESS 			= -160, 	/* 2�����¦�??���䨪?�� */
		DRV_ERR_COMMAND 				= -161, 	/* ?����???�䨪?�� */
		DRV_ERR_PE_ERROR				= -162, 	/* ��?D��䨪?�� */
		DRV_ERR_NAND_BUSY				= -163, 	/* D????| */
		DRV_ERR_PROTECTED				= -164, 	/* ����?��??���� */
		DRV_ERR_BANK_ERROR				= -165, 	/* BANK�䨪?�� */
		DRV_ERR_UNKNOWN 				= -166, 	/* UNKNOWN�䨪?�� */
		DRV_ERR_LENGTH					= -167,
	
		/*-150~-154 for backlight */
		DRV_ERR_INTERNAL_PERIOD 		= -180,
		DRV_ERR_INTERNAL_FREQ			= -181,

		DRV_ERR_MAX		
}TR_DRVS_RETURN_TYPE;


/****************************************************************************
* 	                                        Types
****************************************************************************/

/****************************************************************************
* 	                                        Constants
****************************************************************************/

/****************************************************************************
* 	                                        Global  Variables
****************************************************************************/

/****************************************************************************
* 	                                        Function Prototypes
****************************************************************************/
#define  AMT 1
#ifdef AMT
/*******************************************************************************
 * Function: amt_nv_write
 * Description: AMT NV parameter write interface
 * Parameters: 
 *   Input:	addr -- the specified address  of nv
 *			buf -- the buffer address of the update data to be written
 *			len -- the length of the update data to be written
 *			
 *   Output:
 *
 * Returns:    0  -- sucess
 *                -4 -- the input parameter is invalid
 *                -5 -- failed to malloc memory 
 * Others: 
 ********************************************************************************/
int  amt_nv_write(unsigned int addr, unsigned char * buf, unsigned int len);

/*******************************************************************************
 * Function: amt_nv_init
 * Description: AMT NV parameter initialization
 * Parameters: 
 *   Input:
 *			
 *   Output:
 *
 * Returns: 
 *
 * Others: 
 ********************************************************************************/
void amt_nv_init(void);
#endif

/*******************************************************************************
 * Function: amt_nv_read
 * Description: AMT NV parameter read interface
 * Parameters: 
 *   Input:	addr -- the specified address  of nv
 *			buf -- the buffer address  to read
 *			len -- the buffer length  to read
 *
 *   Output:
 *
 * Returns: 
 *
 * Others: 
 ********************************************************************************/
int  amt_nv_read(unsigned int addr, unsigned char * buf, unsigned int len);
#endif

#ifdef TUYA_SDK_ADPT
int amt_mac_write(unsigned char * MAC);
#endif

