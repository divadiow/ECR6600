/********************************** (C) COPYRIGHT *******************************
* File Name          : CH390.c
* Author             : WCH
* Version            : V1.0
* Date               : 2023/05/30
* Description        : CH390 Ethernet controller source file
*******************************************************************************/
#include <string.h>

#include "spi.h"
#include "chip_pinmux.h"
#include "chip_clk_ctrl.h"
#include "dma.h"
#include "chip_irqvector.h"
#include "oshal.h"
#include "cli.h"

#include "tkl_spi.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tkl_mutex.h"

#include "CH390.h"
#include "tkl_spi_ethernet.h"

#define CH390_INTERFACE_TUYA_EXT
ch390_interface_t ch390_if;

extern spi_interface_config_t spi_master;
extern TKL_MUTEX_HANDLE spi_ethernet_lock;

extern void cli_printf(const char *f, ...);

void ch390_interface_register(ch390_interface_t interface)
{
    ch390_if = interface;
}

#ifdef CH390_INTERFACE_SPI
#define ch390_spi_dummy_read() ch390_if.spi_exc_data(0)
#if 1
extern unsigned char ch390_reg_dbg_en;
#endif
/**
 * @name ch390_read_reg
 * @brief Read register
 * @param reg - Target register address
 * @return Register value
 */
#ifdef CH390_INTERFACE_TUYA_EXT
uint8_t ch390_read_reg(uint8_t reg)
{
    int ret;
    uint8_t spi_data;
    uint8_t snd_data[32] = { 0 };
    uint8_t rcv_data[32] = { 0 };

    spi_master.cmd_read = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;
    spi_master.cmd_write = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;

    snd_data[0] = reg | OPC_REG_R;

    SPI_ETHERNET_LOCK();
    ret = spi_master_write_read(snd_data, rcv_data, 1, 1);
    SPI_ETHERNET_UNLOCK();
    if (ret < 0) {
        SPI_LAN_ERR("%s: call spi_master_write_read failed(ret=%d)", __func__, ret);
    }

    spi_data = rcv_data[0];
//#ifdef SPI_LAN_DBG_TRACE
    if (ch390_reg_dbg_en) {
    SPI_LAN_DBG("%s: read register command 0x%02x reg_addr 0x%02x value 0x%x",
            __func__, snd_data[0], reg, spi_data);
    }
//#endif
    return spi_data;
}
#else
uint8_t ch390_read_reg(uint8_t reg)
{
    uint8_t spi_data;
    ch390_if.scs(0);  // CS = 0
    ch390_if.spi_exc_data(reg | OPC_REG_R);
    spi_data = ch390_spi_dummy_read();
    ch390_if.scs(1); // CS = 1
    return spi_data;
}
#endif /* CH390_INTERFACE_TUYA_EXT */

/**
 * @name ch390_write_reg
 * @brief Write register
 * @param reg - Target register address
 * @param value - Value to be written
 */
#ifdef CH390_INTERFACE_TUYA_EXT
void ch390_write_reg(uint8_t reg, uint8_t value)
{
    int ret;
    uint8_t snd_data[32] = { 0 };

    spi_master.cmd_read = SPI_TRANSCTRL_TRAMODE_RO | SPI_TRANSCTRL_CMDDIS;
    spi_master.cmd_write = SPI_TRANSCTRL_TRAMODE_WO | SPI_TRANSCTRL_CMDDIS;

    snd_data[0] = reg | OPC_REG_W;
    snd_data[1] = value;
//#ifdef SPI_LAN_DBG_TRACE
    if (ch390_reg_dbg_en) {
    SPI_LAN_DBG("%s: write register command 0x%02x reg_addr 0x%02x value 0x%x",
            __func__, snd_data[0], reg, snd_data[1]);
    }
//#endif            

	spi_transaction_t config;
	config.cmmand = 0x51;
	config.addr = 0x33445566;
	config.length = 2;

    SPI_ETHERNET_LOCK();
    ret = spi_master_write(snd_data, &config);
    SPI_ETHERNET_UNLOCK(); 
    if (ret < 0) {
        SPI_LAN_ERR("%s: call spi_master_write_read failed(ret=%d)", __func__, ret);
    }
}
#else
void ch390_write_reg(uint8_t reg, uint8_t value)
{
    ch390_if.scs(0); // CS = 0
    ch390_if.spi_exc_data(reg | OPC_REG_W);
    ch390_if.spi_exc_data(value);
    ch390_if.scs(1); // CS = 1
    return;
}
#endif /* CH390_INTERFACE_TUYA_EXT */

/**
 * @name ch390_read_mem
 * @brief Read data from RX SRAM
 * @param data - Data buffer
 * @param length - Length to read
 */
#ifdef CH390_INTERFACE_TUYA_EXT
void ch390_read_mem(uint8_t *data, int length)
{
    int ret;
    uint8_t *rcv_data;
    uint16_t buff_len; 
    uint8_t cmd_buff[32] = { 0 };

    buff_len = length;

    rcv_data = (uint8_t *)tkl_system_malloc(buff_len);
    if (NULL == rcv_data) {
        SPI_LAN_ERR("%s: malloc memory failed", __func__);
        return;
    }

    memset(rcv_data, 0, buff_len);
    cmd_buff[0] = OPC_MEM_READ;

    spi_master.cmd_read = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;
    spi_master.cmd_write = SPI_TRANSCTRL_TRAMODE_WR | SPI_TRANSCTRL_CMDDIS;
#ifdef SPI_LAN_DBG_TRACE
    //SPI_LAN_DBG("%s: snd_data 0x%x read length %d rcv_data %p real_len %d",
    //        __func__, cmd_buff[0], length, rcv_data, buff_len);
#endif
    SPI_ETHERNET_LOCK();
    ret = spi_master_write_read(cmd_buff, rcv_data, 1, buff_len);
    SPI_ETHERNET_UNLOCK();    
    if (ret < 0) {
        SPI_LAN_ERR("%s: call spi_master_write_read failed(ret=%d)", __func__, ret);
    } else {
        memcpy(data, rcv_data, length);
#ifdef SPI_LAN_DBG_TRACE
        //if (buff_len > 4) {
        //    SPI_LAN_DBG("%s: rcv_data[0] %02x rcv_data[1] %02x rcv_data[2] %02x rcv_data[3] %02x",
        //           __func__, rcv_data[0], rcv_data[1], rcv_data[2], rcv_data[3]);
        //}
#endif /* SPI_LAN_DBG_EN */ 
    }
    
    tkl_system_free(rcv_data);
}
#else
void ch390_read_mem(uint8_t *data, int length)
{
	int i;
    ch390_if.scs(0);
    ch390_if.spi_exc_data(OPC_MEM_READ);

    for(i = 0; i < length; i++) {
        data[i] = ch390_spi_dummy_read();
    }
    ch390_if.scs(1);
}
#endif /* CH390_INTERFACE_TUYA_EXT */
/**
 * @name ch390_write_mem
 * @brief Write data to TX SRAM
 * @param data - Data buffer
 * @param length - Length to write
 */
#ifdef CH390_INTERFACE_TUYA_EXT
void ch390_write_mem(uint8_t *data, int length)
{
    int ret;
    uint8_t *snd_data;
    uint8_t *ptr;
    uint16_t buff_len;
    spi_transaction_t trans_desc;

    spi_master.cmd_read = SPI_TRANSCTRL_TRAMODE_RO | SPI_TRANSCTRL_CMDDIS;
    spi_master.cmd_write = SPI_TRANSCTRL_TRAMODE_WO | SPI_TRANSCTRL_CMDDIS;

    buff_len = length + 1;

    snd_data = (uint8_t *)tkl_system_malloc(buff_len);
    if (NULL == snd_data) {
        SPI_LAN_ERR("%s: malloc memory failed", __func__);
        return;
    }
    memset(snd_data, 0, buff_len);

    snd_data[0] = OPC_MEM_WRITE;
    ptr = snd_data + 1;
    memcpy(ptr, data, length);
  
	trans_desc.cmmand = 0x51;
	trans_desc.addr = 0x33445566;
	trans_desc.length = buff_len;

    SPI_ETHERNET_LOCK();
    ret = spi_master_write(snd_data, &trans_desc);
    SPI_ETHERNET_UNLOCK();    
    if (ret < 0) {
        SPI_LAN_ERR("%s: call spi_master_write_read failed(ret=%d)", __func__, ret);
    } 

    tkl_system_free(snd_data);
}
#else
void ch390_write_mem(uint8_t *data, int length)
{
	int i;
    ch390_if.scs(0);
    ch390_if.spi_exc_data(OPC_MEM_WRITE);
    for(i = 0; i < length; i++) {
        ch390_if.spi_exc_data(data[i]);
    }
    ch390_if.scs(1);
}
#endif /* CH390_INTERFACE_TUYA_EXT */

uint16_t ch390_MWR(void)
{
    uint16_t addr = 0;
    addr = ch390_read_reg(CH390_MWRL);
    addr |= (ch390_read_reg(CH390_MWRH)<<8);
    return addr;
}
#else
/**
 * @name ch390_read_reg
 * @brief Read register
 * @param reg - Target register address
 * @return Register value
 */
uint8_t ch390_read_reg(uint8_t reg)
{
		uint8_t data;
    // Set index
    ch390_if.cs(0);
    ch390_if.cmd(0);
    ch390_if.wrb(0);
    ch390_if.ll_write(reg);
    ch390_if.wrb(1);
    ch390_if.cmd(1);

    // Read data
    ch390_if.ll_ioinput();
    ch390_if.rdb(0);
    data = (uint8_t)(ch390_if.ll_read());
    ch390_if.rdb(1);
    ch390_if.cs(1);
    return data;
}

/**
 * @name ch390_write_reg
 * @brief Write register
 * @param reg - Target register address
 * @param value - Value to be written
 */
void ch390_write_reg(uint8_t reg, uint8_t value)
{
    // Set index
    ch390_if.cs(0);
    ch390_if.cmd(0);
    ch390_if.wrb(0);
    ch390_if.ll_write(reg);
    ch390_if.wrb(1);
    ch390_if.cmd(1);

    // Write data
    ch390_if.wrb(0);
    ch390_if.ll_write(value);
    ch390_if.wrb(1);
    ch390_if.cs(1);
}

void set_index(uint16_t reg)
{
    ch390_if.cs(0);
    ch390_if.cmd(0);
    ch390_if.wrb(0);
    ch390_if.ll_write(reg);
    ch390_if.wrb(1);
    ch390_if.cmd(1);
    ch390_if.cs(1);
}

#ifdef CH390_INTERFACE_8_BIT
/**
 * @name ch390_read_mem
 * @brief Read data from RX SRAM
 * @param data - Data buffer
 * @param length - Length to read
 */
void ch390_read_mem(uint8_t *data, int length)
{
    int i;
    set_index(CH390_MRCMD);
    ch390_if.ll_ioinput();
    ch390_if.cs(0);
    for(i = 0; i < length; i++) {
        ch390_if.rdb(0);
        data[i] = ch390_if.ll_read();
        ch390_if.rdb(1);
    }
    ch390_if.cs(1);
}
/**
 * @name ch390_write_mem
 * @brief Write data to TX SRAM
 * @param data - Data buffer
 * @param length - Length to write
 */
void ch390_write_mem(uint8_t *data, int length)
{
	int i;
    set_index(CH390_MWCMD);
    ch390_if.cs(0);
    for(i = 0; i < length; i++) {
        ch390_if.wrb(0);
        ch390_if.ll_write(data[i]);
        ch390_if.wrb(1);
    }
    ch390_if.cs(1);
}
#endif // CH390_INTERFACE_8_BIT
#ifdef CH390_INTERFACE_16_BIT
void ch390_read_mem(uint8_t *data, int length)
{
    int i;
    uint16_t *data_w = (uint16_t *)data;
    length = (length + 1) / 2;

    set_index(CH390_MRCMD);
    ch390_if.ll_ioinput();
    ch390_if.cs(0);
    for(i = 0; i < length; i++) {
        ch390_if.rdb(0);
        data_w[i] = ch390_if.ll_read();
        ch390_if.rdb(1);
    }
    ch390_if.cs(1);
}
void ch390_write_mem(uint8_t *data, int length)
{
    int i;
    uint16_t *data_w = (uint16_t *)data;
    length = (length + 1) / 2;

    set_index(CH390_MWCMD);
    ch390_if.cs(0);
    for(i = 0; i < length; i++) {
        ch390_if.wrb(0);
        ch390_if.ll_write(data_w[i]);
        ch390_if.wrb(1);
    }
    ch390_if.cs(1);
}
#endif // CH390_INTERFACE_16_BIT
#endif // CH390_INTERFACE_SPI

/**
 * @name ch390_receive_packet
 * @brief Receive packet
 * @param buff - Size equal to CH390_PKT_MAX
 * @param rx_status - Output abnormal status while receiving packet.
 *                    It has the same meaning as RSR(06h).
 * @return Packet length
 */
uint32_t ch390_receive_packet(uint8_t *buff, uint8_t *rx_status)
{
    uint8_t rx_ready;
    uint16_t rx_len = 0;
	uint8_t ReceiveData[4];

    // Check packet ready or not
    ch390_read_reg(CH390_MRCMDX);
    rx_ready = ch390_read_reg(CH390_MRCMDX);

    // if rxbyte != 1 or 0 reset device
    if (rx_ready & CH390_PKT_ERR)
    {
        // Reset RX FIFO pointer
        ch390_write_reg(CH390_RCR, 0);        //RX disable
        ch390_write_reg(CH390_MPTRCR, 0x01);  //Reset RX FIFO pointer
        ch390_write_reg(CH390_MRRH, 0x0c);
        ch390_if.delay_us(1000);
        ch390_write_reg(CH390_RCR, RCR_RXEN); //RX Enable
        return 0;
    }
    if (!(rx_ready & CH390_PKT_RDY))
    {
        return 0;
    }

    ch390_read_mem(ReceiveData, 4);

    *rx_status = ReceiveData[1];
    rx_len = ReceiveData[2] | (ReceiveData[3] << 8);

    if(rx_len <= CH390_PKT_MAX)
    {
        ch390_read_mem(buff, rx_len);
    }

    if ((*rx_status & 0x3f) || (rx_len > CH390_PKT_MAX))
    {
        return 0;
    }
    return rx_len;
}

/**
 * @name ch390_send_packet
 * @brief Send packet
 * @param buff - Data to be sent
 * @param length - Less than 3k bytes.
 */
void ch390_send_packet(uint8_t *buff, uint16_t length)
{
    // Write data to SRAM
    ch390_write_mem(buff, length);
    // Wait until last transmit complete
    while(ch390_read_reg(CH390_TCR) & TCR_TXREQ);
    // Set current packet length
    ch390_write_reg(CH390_TXPLL, length & 0xff);
    ch390_write_reg(CH390_TXPLH, (length >> 8) & 0xff);
    // Issue transmit request
    ch390_send_request();
}

/**
 * @name ch390_send_request
 * @brief Issue transmit request
 */
void ch390_send_request()
{
    uint8_t tcr = ch390_read_reg(CH390_TCR);
    ch390_write_reg(CH390_TCR, tcr | TCR_TXREQ);
}

/**
 * @name ch390_drop_packet
 * @brief Drop packet in RX SRAM if don't want to read it. This function
 *        modify the memory data read pointer and skip specified length
 * @param len - Skip length, length of the current packet.
 */
void ch390_drop_packet(uint16_t len)
{
    uint16_t mdr = ch390_read_reg(CH390_MRRL) | (ch390_read_reg(CH390_MRRH) << 8);
#ifdef CH390_INTERFACE_16_BIT
    mdr = mdr + (len + 1) / 2 * 2;
#else
    mdr = mdr + len;
#endif
    mdr = mdr < 0x4000 ? mdr : mdr - 0x3400;
    ch390_write_reg(CH390_MRRL, mdr & 0xff);
    ch390_write_reg(CH390_MRRH, (mdr >> 8) & 0xff);
}

/**
 * @name ch390_read_phy
 * @brief Read PHY register
 * @param reg - PHY register address
 */
uint16_t ch390_read_phy(uint8_t reg)
{
    ch390_write_reg(CH390_EPAR, CH390_PHY | reg);
    // Chose PHY, send read command
    ch390_write_reg(CH390_EPCR, EPCR_ERPRR | EPCR_EPOS);
    ch390_if.delay_us(100);
    // Clear read command
    ch390_write_reg(CH390_EPCR, 0x00);
    return (ch390_read_reg(CH390_EPDRH) << 8) |
           (ch390_read_reg(CH390_EPDRL) & 0xFF);
}

/**
 * @name ch390_write_phy
 * @brief Write PHY register
 * @param reg - PHY register address
 * @param value - Value to be written
 */
void ch390_write_phy(uint8_t reg, uint16_t value)
{
    ch390_write_reg(CH390_EPAR, CH390_PHY | reg);
    ch390_write_reg(CH390_EPDRL, (value & 0xff));        // Low byte
    ch390_write_reg(CH390_EPDRH, ((value >> 8) & 0xff)); // High byte
    // Chose PHY, send write command
    ch390_write_reg(CH390_EPCR, 0x0A);
    ch390_if.delay_us(100);
    // Clear write command
    ch390_write_reg(CH390_EPCR, 0x00);
}

/**
 * @name ch390_write_eeprom
 * @brief Write EEPROM register
 * @param reg - EEPROM register address
 * @param value - Value to be written
 */
void ch390_write_eeprom(uint8_t reg, uint16_t value)
{
    ch390_write_reg(CH390_EPAR, reg);
    ch390_write_reg(CH390_EPDRL, (value & 0xff));        // Low byte
    ch390_write_reg(CH390_EPDRH, ((value >> 8) & 0xff)); // High byte
    // Chose EEPROM, send write command
    ch390_write_reg(CH390_EPCR, EPCR_ERPRW);
    // Clear write command
    while(ch390_read_reg(CH390_EPCR) & 0x01);
    // Clear write command
    ch390_write_reg(CH390_EPCR, 0x00);
}

/**
 * @name ch390_software_reset
 * @brief Software reset CH390 by NCR
 */
void ch390_software_reset()
{
    ch390_write_reg(CH390_NCR, NCR_RST);
    ch390_if.delay_us(10);
    ch390_write_reg(CH390_NCR, 0);
    ch390_write_reg(CH390_NCR, NCR_RST);
    ch390_if.delay_us(10);
}

/**
 * @name ch390_hardware_reset
 * @brief Hardware reset CH390 by pull down RSTB pin
 */
void ch390_hardware_reset()
{
    ch390_if.rst(0);
    ch390_if.delay_us(10);
    ch390_if.rst(1);
    ch390_if.delay_us(10000);
}

/**
 * @name ch390_default_config
 * @brief Config CH390 with default options:
 *        LED mode 1;
 *        Enable transmit check sum generation;
 *        Enable RX;
 *        Enable all interrupt and PAR
 */
void ch390_default_config()
{
    // MAC address
//    uint8_t mac_addr[6] = { 0x50, 0x54, 0x7B, 0x84, 0x00, 0x73 };
    // Multicast address hash table
    uint8_t multicase_addr[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    ch390_set_phy_mode(CH390_AUTO);

    // Clear status
    ch390_write_reg(CH390_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);
    ch390_write_reg(CH390_ISR, 0xFF);    // Clear interrupt status
    ch390_write_reg(CH390_TCR2, 0x80);        // LED mode 1
    ch390_write_reg(CH390_TCSCR, TCSCR_ALL);  // Enable check sum generation

//    ch390_set_mac_address(mac_addr);
    ch390_set_multicast(multicase_addr);

    // Enable all interrupt and PAR
    ch390_write_reg(CH390_IMR, IMR_ALL);
    // Enable RX
    ch390_write_reg(CH390_RCR, RCR_RXEN);
}

/**
 * @name ch390_set_phy_mode
 * @brief Set PHY mode and enable PHY.
 *        PHY mode: Auto-negotiation, 10M/100M, full-duplex/half-duplex
 * @param mode - PHY mode
 */
void ch390_set_phy_mode(enum ch390_phy_mode mode)
{
    uint16_t BMCR_value = 0;
    uint16_t ANAR_value = 0;
    switch (mode)
    {
    case CH390_10MHD:
        BMCR_value = 0x1000;
        ANAR_value = 0x21;
        break;
    case CH390_10MFD:
        BMCR_value = 0x1100;
        ANAR_value = 0x41;
        break;
    case CH390_100MHD:
        BMCR_value = 0x3000;
        ANAR_value = 0x81;
        break;
    case CH390_100MFD:
        BMCR_value = 0x3100;
        ANAR_value = 0x101;
        break;
    case CH390_AUTO:
        BMCR_value = 0x1000;
        ANAR_value = 0x01E1;
        break;
    }

    ch390_write_phy(CH390_PHY_BMCR, BMCR_value);
    ch390_write_phy(CH390_PHY_ANAR, ANAR_value);
    ch390_write_reg(CH390_GPR, 0x00);   // Enable PHY
}

/**
 * @name ch390_set_mac_address
 * @brief Set mac address
 * @param mac_addr - 6-byte length mac address array
 */
void ch390_set_mac_address(uint8_t *mac_addr)
{
    uint8_t i;
    for (i = 0; i < 6; i++)
    {
        ch390_write_reg(CH390_PAR + i, mac_addr[i]);
    }
}

/**
 * @name ch390_set_multicast
 * @brief Set multicast address hash table
 * @param multicast_addr - 8-byte length multicast address hash table array
 */
void ch390_set_multicast(uint8_t *multicast_hash)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        ch390_write_reg(CH390_MAR + i, multicast_hash[i]);
    }
}

/**
 * @brief reflect an 8bit value.
 *        Only for "ch390_compute_hash_bit"
 */
static uint8_t reflect_8(uint8_t val)
{
    uint8_t resVal = 0;

    for (int i = 0; i < 8; i++)
    {
        if ((val & (1 << i)) != 0)
        {
            resVal |= 1 << (7 - i);
        }
    }
    return resVal;
}

/**
 * @brief Calculate the corresponding hash bit of the MAC address.
 *        Only for "ch390_set_hash_bit"
 * @param mac - Destination address
 * @return Hash bit number
 */
static uint8_t ch390_compute_hash_bit(uint8_t *mac)
{
    const uint32_t poly = 0x4C11DB7;
    uint32_t crc = 0xffffffff;

    int byte_i = 0;
    for(byte_i = 0; byte_i < 6; byte_i++)
    {
        uint8_t cur_byte = reflect_8(mac[byte_i]);
        crc ^= cur_byte << 24;
        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x80000000) != 0)
            {
                crc = (crc << 1) ^ poly;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return (crc ^ 0xffffffff) >> 26;
}

/**
 * @brief Set MAR bit for a particular MAC address
 * @param mac - Destination address
 */
void ch390_set_hash_bit(uint8_t *mac)
{
    uint8_t bit = ch390_compute_hash_bit(mac);
    uint8_t mar = CH390_MAR + bit / 8;

    uint8_t mar_val = ch390_read_reg(mar);
    mar_val |= 1 << (bit % 8);
    ch390_write_reg(mar, mar_val);
}

/**
 * @name ch390_get_mac
 * @brief Get mac address
 * @param mac_addr - 6-byte length mac address output
 */
void ch390_get_mac(uint8_t *mac_addr)
{
    uint8_t i;
    for (i = 0; i < 6; i++)
    {
        mac_addr[i] = ch390_read_reg(CH390_PAR + i);
    }
}

/**
 * @name ch390_get_multicast
 * @brief Get multicast address hash table
 * @param multicast_addr - 8-byte length multicast address hash table output
 */
void ch390_get_multicast(uint8_t *multicast_hash)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        multicast_hash[i] = ch390_read_reg(CH390_MAR + i);
    }
}

/**
 * @name ch390_get_vendor_id
 * @brief Get vendor ID
 * @return Vendor ID
 */
uint16_t ch390_get_vendor_id()
{
    uint16_t id;
    id = (ch390_read_reg(CH390_VIDL) & 0xff);
    id |= ch390_read_reg(CH390_VIDH) << 8;
    return id;
}

/**
 * @name ch390_get_product_id
 * @brief Get product ID
 * @return Product ID
 */
uint16_t ch390_get_product_id()
{
    uint16_t id;
    id = (ch390_read_reg(CH390_PIDL) & 0xff);
    id |= ch390_read_reg(CH390_PIDH) << 8;
    return id;
}

/**
 * @name ch390_get_revision
 * @brief Get chip revision
 * @return Chip revision
 */
uint8_t ch390_get_revision()
{
    return ch390_read_reg(CH390_CHIPR);
}

/**
 * @name ch390_interrupt_config
 * @brief Interrupt configuration
 * @param mask - Interrupt to be enabled, see "CH390.h" IMR_xxx
 */
void ch390_interrupt_config(uint8_t mask)
{
    ch390_write_reg(CH390_IMR, mask);
}

/**
 * @name ch390_rx_enable
 * @brief Enable or disable packet receive
 * @param op - 0: disable  1: enable
 */
void ch390_rx_enable(int op)
{
    uint8_t rcr = ch390_read_reg(CH390_RCR);

    if(op == 0)
        rcr &= ~RCR_RXEN;
    else
        rcr |= RCR_RXEN;

    ch390_write_reg(CH390_RCR, rcr);
}

/**
 * @name ch390_rx_filter_config
 * @brief Configure receive filter.
 * @param config - See "CH390.h" RCR_xxx
 */
void ch390_rx_filter_config(uint8_t config)
{
    uint8_t rcr = ch390_read_reg(CH390_RCR) & RCR_RXEN;
    ch390_write_reg(CH390_RCR, rcr | config);
}

/**
 * @name ch390_wakeup_config
 * @brief Enable or disable wakeup_function
 * @param events - Events that trigger wakeup
 *                 WCR_LINKEN   - Link status change
 *                 WCR_SAMPLEEN - Sample frame
 *                 WCR_MAGICEN  - Magic packet
 *                 0            - Disable wakeup function
 */
void ch390_wakeup_config(uint8_t events)
{
    uint8_t ncr = ch390_read_reg(CH390_NCR);
    if(events)
        ncr |= NCR_WAKEEN;
    else {
        ncr &= ~NCR_WAKEEN;
    }
    ch390_write_reg(CH390_NCR, ncr);
    ch390_write_reg(CH390_WCR, events);
}

/**
 * @name ch390_wake_notify
 * @brief Wait for Magic Packet or Sample Frame and discard all
 *        other packets.
 *        If the application needs to use Wake On LAN, call this
 *        function every time before MCU enters low power mode.
 *        An external interrupt signal is accessible on WOL pin
 *        when wake up event occurred.
 */
void ch390_wake_notify()
{
    uint8_t ncr = ch390_read_reg(CH390_NCR);
    ch390_write_reg(CH390_NCR, ncr ^ 0x10);
}

/**
 * @name ch390_loop_back_enable
 * @brief Enable loop back mode
 * @param op - 0: disable  1: enable
 */
void ch390_loop_back_enable(int op)
{
    uint8_t ncr = ch390_read_reg(CH390_NCR) & ~0x06;

    if(op == 1) ncr |= NCR_LBK_MAC;

    ch390_write_reg(CH390_NCR, ncr);
}

/**
 * @name ch390_get_duplex_mode
 * @brief Get current duplex mode of the internal PHY
 * @return 0: Half-duplex  1: Full-duplex
 */
int ch390_get_duplex_mode()
{
    return !!(ch390_read_reg(CH390_NCR) & NCR_FDX);
}

/**
 * @name ch390_get_phy_speed
 * @brief Get the speed of the internal PHY.
 *        Only valid after PHY linked
 * @return 0: 100Mbps  1: 10Mbps
 */
int ch390_get_phy_speed()
{
    return !!(ch390_read_reg(CH390_NSR) & NSR_SPEED);
}

/**
 * @name ch390_get_link_status
 * @brief Get link status of the internal PHY
 * @return 0: Link failed  1: Link OK
 */
int ch390_get_link_status()
{
    uint8_t nsr = ch390_read_reg(CH390_NSR);
    return !!(nsr & NSR_LINKST);
}

/**
 * @name ch390_sleep_control
 * @brief Enter or exit sleep mode
 * @param op - 0: Power up  1: Power down
 */
void ch390_sleep_control(int op)
{
    if(op)
    {
        ch390_write_reg(CH390_SCCR, 0x01);
    }
    else
    {
        ch390_read_reg(CH390_RSCCR);
        ch390_if.delay_us(100);
    }

}

#ifndef CH390_INTERFACE_16_BIT
/**
 * @name ch390_gpio_config
 * @brief Config the input/output direction of GPIO1~3
 *        In 8-bit mode, GPIO4~6 are output only
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO3
 *        dir - 0: Input  1: Output
 */
void ch390_gpio_config(uint8_t GPIOx, uint8_t dir)
{
    uint8_t gpcr = ch390_read_reg(CH390_GPCR);
    if(dir)
    {
        gpcr |= GPIOx;
    }
    else {
        gpcr &= ~GPIOx;
    }
    ch390_write_reg(CH390_GPCR, gpcr);
}

/**
 * @name ch390_gpio_write_bit
 * @brief Sets or clears the selected gpio bit.
 *        In SPI mode, only GPIO1~3 are available
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO6
 *        level - 0: Clear pin  1: Set pin
 */
void ch390_gpio_write_bit(uint8_t GPIOx, uint8_t level)
{
    uint8_t gpr = ch390_read_reg(CH390_GPR);
    if(level)
    {
        gpr |= GPIOx;
    }
    else {
        gpr &= ~GPIOx;
    }
    ch390_write_reg(CH390_GPR, gpr);
}

/**
 * @name ch390_gpio_read_bit
 * @brief Read gpio input, only CH390_GPIO1 ~ 3 are available
 * @param GPIOx - CH390_GPIO1 ~ CH390_GPIO3
 * @return Input pin value
 */
uint8_t ch390_gpio_read_bit(uint8_t GPIOx)
{
    uint8_t gpr = ch390_read_reg(CH390_GPR);
    return !!(gpr & GPIOx);
}
#endif

/**
 * @name ch390_int_pin_config
 * @brief Configure INT pin output type and polarity
 * @param type - INCR_TYPE_OD: Open drain output
 *               INCR_TYPE_PP: Push pull output
 *        pol - INCR_POL_L: Active low
 *              INCR_POL_H: Active high
 */
void ch390_int_pin_config(uint8_t type, uint8_t pol)
{
    ch390_write_reg(CH390_INTCR, type | pol);
}

/**
 * @name ch390_get_int_status
 * @brief Get CH390 interrupt status and clear them
 * @return Interrupt status
 */
uint8_t ch390_get_int_status()
{
    uint8_t int_status = ch390_read_reg(CH390_ISR);
    // Clear interrupt status by write 1
    ch390_write_reg(CH390_ISR, int_status);
    return int_status;
}

void ch390_reg_dump(void)
{
    uint8_t var;

    var = ch390_read_reg(CH390_ISR);
    SPI_LAN_LOG("%s: CH390_ISR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_INTCR);
    SPI_LAN_LOG("%s: CH390_INTCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_NSR);
    SPI_LAN_LOG("%s: CH390_NSR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_MRCMDX);
    SPI_LAN_LOG("%s: CH390_MRCMDX 0x%x", __func__, var);

    var = ch390_read_reg(CH390_IMR);
    SPI_LAN_LOG("%s: CH390_IMR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TCR);
    SPI_LAN_LOG("%s: CH390_TCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TXPLL);
    SPI_LAN_LOG("%s: CH390_TXPLL 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TXPLH);
    SPI_LAN_LOG("%s: CH390_TXPLH 0x%x", __func__, var);
    
    var = ch390_read_reg(CH390_TCR2);
    SPI_LAN_LOG("%s: CH390_TCR2 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TCSCR);
    SPI_LAN_LOG("%s: CH390_TCSCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_RCR);
    SPI_LAN_LOG("%s: CH390_RCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_EPAR);
    SPI_LAN_LOG("%s: CH390_EPAR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_GPR);
    SPI_LAN_LOG("%s: CH390_GPR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_PHY_BMCR);
    SPI_LAN_LOG("%s: CH390_PHY_BMCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_PHY_ANAR);
    SPI_LAN_LOG("%s: CH390_PHY_ANAR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_RSR);
    SPI_LAN_LOG("%s: CH390_RSR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_GPCR);
    SPI_LAN_LOG("%s: CH390_GPCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_ATCR);
    SPI_LAN_LOG("%s: CH390_ATCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_SBCR);
    SPI_LAN_LOG("%s: CH390_SBCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_MPAR);
    SPI_LAN_LOG("%s: CH390_MPAR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_WCR);
    SPI_LAN_LOG("%s: CH390_WCR 0x%x", __func__, var);
    
    var = ch390_read_reg(CH390_EPDRL);
    SPI_LAN_LOG("%s: CH390_EPDRL 0x%x", __func__, var);

    var = ch390_read_reg(CH390_EPDRH);
    SPI_LAN_LOG("%s: CH390_EPDRH 0x%x", __func__, var);

    var = ch390_read_reg(CH390_EPCR);
    SPI_LAN_LOG("%s: CH390_EPCR 0x%x", __func__, var);

    var = ch390_read_reg(CH390_EPCR);
    SPI_LAN_LOG("%s: CH390_EPCR2 0x%x", __func__, var);

    var = ch390_read_reg(CH390_EPCR);
    SPI_LAN_LOG("%s: CH390_EPCR3 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TRPAL);
    SPI_LAN_LOG("%s: CH390_TRPAL 0x%x", __func__, var);

    var = ch390_read_reg(CH390_TRPAH);
    SPI_LAN_LOG("%s: CH390_TRPAH 0x%x", __func__, var);

    var = ch390_read_reg(CH390_MWRL);
    SPI_LAN_LOG("%s: CH390_MWRL 0x%x", __func__, var);

    var = ch390_read_reg(CH390_MWRH);
    SPI_LAN_LOG("%s: CH390_MWRH 0x%x", __func__, var);
}
