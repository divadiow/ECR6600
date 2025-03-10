#ifndef _SDIO_SLAVE_INTERFACE_H_
#define _SDIO_SLAVE_INTERFACE_H_

#define MAX_BUF_NUM_PER_TIME  4

typedef struct _sdio_bufs
{
	struct _sdio_bufs * next;
	unsigned int bufs[MAX_BUF_NUM_PER_TIME];
	unsigned int lens[MAX_BUF_NUM_PER_TIME];
	unsigned char  need_free[MAX_BUF_NUM_PER_TIME];  // only have means when sdio tx.
	unsigned char  nb;
}sdio_bufs_t;

typedef struct _sdio_tx_bufs
{
	struct _sdio_tx_bufs * next_buf;
	struct _sdio_tx_bufs * next_frag;
	unsigned int lens;
	unsigned int bufs;
	unsigned int total_len;
}sdio_tx_bufs_t;

typedef void (*sdio_tx_cb_t)(void*);

typedef struct
{
	sdio_tx_bufs_t bufs; // must be first member
	sdio_tx_cb_t cb;
	void     *param;
	void     *rxdesc;
}sdio_tx_param_t;

/*******************************************************************************
 * Function: sdio_send_from_mac
 * Description: send msg/dbg/frame from mac to sdio host
 * Parameters: 
 *   Input:bufs: buff array, lens: len array, nb: number of buff
 *
 *   Output:
 *
 * Returns: true or false
 *
 *
 * Others: 
 ********************************************************************************/
 unsigned int sdio_send_to_host(sdio_tx_param_t *param);
 unsigned int sdio_tx_queue_count(void);


 /*******************************************************************************
  * Function: sdio_tx_callback
  * Description: send msg/dbg/frame callback when complete
  * Parameters: 
  *   Input:bufs: buff array
  *
  *   Output:
  *
  * Returns: true or false
  *
  *
  * Others: 
  ********************************************************************************/
	 void sdio_tx_callback(void *para);
 
 /*******************************************************************************
 * Function: mac_rx_cb
 * Description:as slave sdio callback
 * Parameters: 
 *   Input: void *buffer, int len, int flag
 *
 *   Output:
 *
 * Returns: 0
 *
 *
 * Others: 
 ********************************************************************************/

int sdio_rx_cb(sdio_bufs_t *bufs, uint32_t flag);

/*******************************************************************************
 * Function: alloc_rx_buffer_for_sdio
 * Description:alloc hw buffer for sdio slave
 * Parameters: 
 *   Input: int flag
 *
 *   Output:
 *
 * Returns: 0
 *
 *
 * Others: 
 ********************************************************************************/
 int alloc_buffer_for_sdio(sdio_bufs_t *bufs, uint32_t flag, uint32_t need_len);
 sdio_tx_bufs_t *sdio_tx_free_queue_pop();
 #endif
