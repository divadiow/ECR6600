#ifndef __TKL_SPI_ETHERNET_H__
#define __TKL_SPI_ETHERNET_H__

#define SPI_LAN_DBG_EN
//#define SPI_LAN_DBG_TRACE
//#define SPI_LAN_DEMO_BORAD_EN
//#define SPI_LAN_HARDWARE_RESET_EN

#define SPI_ETHERNET_IFNAME0 'e'
#define SPI_ETHERNET_IFNAME1 'n'

#define SPI_LAN_LOCK_SUPPORT
#define SPI_ETHERNET_THREAD_SIZE 4096
#define SPI_ETHERNET_THREAD_PRIORITY 3

#ifdef SPI_LAN_DBG_EN
#define SPI_LAN_DBG(fmt, args...) cli_printf(fmt"\r\n", args)
#define SPI_LAN_LOG(fmt, args...) cli_printf(fmt"\r\n", args)
#define SPI_LAN_ERR(fmt, args...) cli_printf(fmt"\r\n", args)
#else
#define SPI_LAN_DBG(fmt, args...) do{}while(0)
#define SPI_LAN_LOG(fmt, args...) cli_printf(fmt"\r\n", args)
#define SPI_LAN_ERR(fmt, args...) cli_printf(fmt"\r\n", args)
#endif /* SPI_LAN_DBG_EN */

#define SPI_ETHERNET_QUEUE_NUM 500
#define CH390_THRAD_NAME "spi_ethernet_proc"

#ifdef SPI_LAN_LOCK_SUPPORT
#define SPI_ETHERNET_LOCK_X() tkl_mutex_lock(spi_ethernet_lock)
#define SPI_ETHERNET_UNLOCK_X()  tkl_mutex_unlock(spi_ethernet_lock)
#define SPI_ETHERNET_RELEASE() tkl_mutex_release(spi_ethernet_lock)
#define SPI_ETHERNET_LOCK()
#define SPI_ETHERNET_UNLOCK()
//#define SPI_ETHERNET_RELEASE()
#else
#define SPI_ETHERNET_LOCK()
#define SPI_ETHERNET_UNLOCK()
#define SPI_ETHERNET_RELEASE()
#endif /* SPI_LAN_LOCK_SUPPORT */

typedef struct {
    unsigned int    tx_ok;
    unsigned int    tx_fail;
    unsigned int    rx_ok;
    unsigned int    rx_fail;
    unsigned int    rx_drop;
} spi_ethernet_txrx_stats_t;

int spi_ethernet_init(void);
#endif /* __TKL_SPI_ETHERNET_H__ */