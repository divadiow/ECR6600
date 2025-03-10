// Copyright 2018-2019 trans-semi inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef _WIFI_SNIFFER_H_
#define _WIFI_SNIFFER_H_

#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Received packet radio metadata header, this is the common header at the beginning of all promiscuous mode RX callback buffers */
typedef struct {
    union {
        struct {
            // RxVcetor1, byte 0
            uint32_t format        : 4;
            uint32_t ch_bandwidth  : 3;
            uint32_t preamble_type : 1;

            // RxVcetor1, byte 1
            uint32_t antenna       : 8;

            // RxVcetor1, byte 2
            int8_t rssi_legacy     : 8;

            // RxVcetor1, byte 3
            uint32_t l_length_l    : 8;

            // RxVcetor1, byte 4
            uint32_t l_length_h    : 4;
            uint32_t l_rate        : 4;

            // RxVcetor1, byte 5
            int8_t rssi            : 8;

            union {
                //non-ht non-ht-dump-ofdm
                struct {
                    // RxVcetor1, byte 6
                    uint32_t dyn_bandwidth_in_non_ht : 1;
                    uint32_t ch_bandwidth_in_non_ht  : 2;
                    uint32_t reserved0               : 4;
                    uint32_t l_sig_valid             : 1;
                }__attribute__((packed));

                //ht-mm ht-gf
                struct {
                    // RxVcetor1, byte 6
                    uint32_t sounding     : 1;
                    uint32_t smoothing    : 1;
                    uint32_t gi_type      : 1;
                    uint32_t aggregation  : 1;
                    uint32_t stbc         : 1;
                    uint32_t num_ext_ss   : 2;
                    uint32_t l_sig_valid2 : 1;

                    // RxVcetor1, byte 7
                    uint32_t mcs          : 7;
                    uint32_t fec          : 1;

                    // RxVcetor1, byte 8
                    uint32_t length_l     : 8;

                    // RxVcetor1, byte 9
                    uint32_t length_h     : 8;
                }__attribute__((packed));

                //vht...

                //he
                struct {
                    // RxVcetor1, byte 6
                    uint32_t sounding2     : 1;
                    uint32_t beamformed    : 1;
                    uint32_t gi_type2      : 2;
                    uint32_t stbc2         : 1;
                    uint32_t reserved8     : 3;

                    // RxVcetor1, byte 7
                    uint32_t uplink_flag   : 1;
                    uint32_t beam_change   : 1;
                    uint32_t dcm           : 1;
                    uint32_t he_ltf_type   : 2;
                    uint32_t doppler       : 1;
                    uint32_t reserved9     : 2;

                    // RxVcetor1, byte 8
                    uint32_t bss_color     : 6;
                    uint32_t reserved10    : 2;

                    // RxVcetor1, byte 9
                    uint32_t txop_duration : 7;
                    uint32_t reserved11    : 1;

                    union {
                        //he-su he-ext-su
                        struct {
                            // RxVcetor1, byte 10
                            uint32_t pe_duration   : 4;
                            uint32_t spatial_reuse : 4;

                            // RxVcetor1, byte 11
                            uint32_t reserved12    : 8;

                            // RxVcetor1, byte 12
                            uint32_t mcs2          : 4;
                            uint32_t nss           : 3;
                            uint32_t fec_coding    : 1;

                            // RxVcetor1, byte 13
                            uint32_t length_l2     : 8;

                            // RxVcetor1, byte 14
                            uint32_t length_m2     : 8;

                            // RxVcetor1, byte 15
                            uint32_t length_h2     : 4;
                            uint32_t reserved13    : 4;
                        }__attribute__((packed));
                        //he-mu
                        struct {
                            // RxVcetor1, byte 10
                            uint32_t pe_duration2           : 4;
                            uint32_t spatial_reuse2         : 4;

                            // RxVcetor1, byte 11
                            uint32_t sig_b_compression_mode : 1;
                            uint32_t dcm_sig_b              : 1;
                            uint32_t mcs_sig_b              : 3;
                            uint32_t ru_size                : 3;

                            // RxVcetor1, byte 12
                            uint32_t mcs3                   : 4;
                            uint32_t nss3                   : 3;
                            uint32_t fec_coding3            : 1;

                            // RxVcetor1, byte 13
                            uint32_t length_l3              : 8;

                            // RxVcetor1, byte 14
                            uint32_t length_m3              : 8;

                            // RxVcetor1, byte 15
                            uint32_t length_h3              : 4;
                            uint32_t reserved14             : 4;
                        }__attribute__((packed));

                        //he-tb(as sta)
                        struct {
                            // RxVcetor1, byte 10
                            uint32_t spatial_reuse3 : 8;

                            // RxVcetor1, byte 11
                            uint32_t spatial_reuse4 : 8;
                        }__attribute__((packed));

                        //he-tb(as ap)
#if 0
                        struct {
                            // RxVcetor1, byte 10
                            uint32_t reserved15 : 8;

                            // RxVcetor1, byte 11
                            uint32_t n_user     : 8;

                            // RxVcetor1, byte 12
                            uint32_t mcs        : 4;
                            uint32_t nss        : 3;
                            uint32_t fec_coding : 1;

                            // RxVcetor1, byte 13
                            uint32_t length_l   : 8;

                            // RxVcetor1, byte 14
                            uint32_t length_m   : 8;

                            // RxVcetor1, byte 15
                            uint32_t length_h   : 4;
                            uint32_t reserved16 : 4;

                            // RxVcetor1, byte 16
                            uint32_t staid      : 8;

                            // RxVcetor1, byte 17
                            uint32_t ataid      : 3;
                            uint32_t reserved17 : 5;
                        }__attribute__((packed));
#endif
                        //he-ndp-trig...
#if 0
                        struct {
                            // RxVcetor1, byte 10
                            uint32_t reserved15 : 8;

                            // RxVcetor1, byte 11
                            uint32_t n_user     : 8;

                            // RxVcetor1, byte 12
                            uint32_t ndp_report : 8;

                            ......
                        }__attribute__((packed));
#endif

                    };
                }__attribute__((packed));
            };

            // RxVcetor2, byte 0
            uint32_t rcpi1 : 8;

            // RxVcetor2, byte 1
            uint32_t rcpi2 : 8;

            // RxVcetor2, byte 2
            uint32_t rcpi3 : 8;

            // RxVcetor2, byte 3
            uint32_t rcpi4 : 8;

            // RxVcetor2, byte 4
            uint32_t evm1  : 8;

            // RxVcetor2, byte 5
            uint32_t evm2  : 8;

            // RxVcetor2, byte 6
            uint32_t evm3  : 8;

            // RxVcetor2, byte 7
            uint32_t evm4  : 8;
        }__attribute__((packed));
        uint32_t   rx_vector[6];
    };
} __attribute__((packed)) wifi_pkt_rx_ctrl_t;

/** @brief Payload passed to 'buf' parameter of promiscuous mode RX callback.
 */
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl; /**< metadata header */
    uint8_t *payload;       /**< Data or management payload */
} wifi_promiscuous_pkt_t;

/**
  * @brief Promiscuous frame type
  *
  * Passed to promiscuous mode RX callback to indicate the type of parameter in the buffer.
  *
  */
 
typedef enum {
    WIFI_PKT_MGMT,  /**< Management frame, indicates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_CTRL,  /**< Control frame, indicates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_DATA,  /**< Data frame, indiciates 'buf' argument is wifi_promiscuous_pkt_t */
    WIFI_PKT_NULL,  /**< invalid frame type, such as unsupported HT frame (only the length can be trusted) */
} wifi_promiscuous_pkt_type_t;

#define WIFI_PROMIS_FILTER_MASK_ALL         (0xFFFFFFFF)  /**< filter all packets */
#define WIFI_PROMIS_FILTER_MASK_MGMT        (1)           /**< filter the packets with type of WIFI_PKT_MGMT */
#define WIFI_PROMIS_FILTER_MASK_CTRL        (1<<1)        /**< filter the packets with type of WIFI_PKT_CTRL */
#define WIFI_PROMIS_FILTER_MASK_DATA        (1<<2)        /**< filter the packets with type of WIFI_PKT_DATA */
#define WIFI_PROMIS_FILTER_MASK_DATA_MPDU   (1<<3)        /**< filter the MPDU which is a kind of WIFI_PKT_DATA */
#define WIFI_PROMIS_FILTER_MASK_DATA_AMPDU  (1<<4)        /**< Todo: filter the AMPDU which is a kind of WIFI_PKT_DATA */

/** @brief Mask for filtering different packet types in promiscuous mode. */
typedef struct {
    uint32_t filter_mask; /**< OR of one or more filter values WIFI_PROMIS_FILTER_* */
} wifi_promiscuous_filter_t;

/**
  * @brief The RX callback function in the promiscuous mode. 
  *        Each time a packet is received, the callback function will be called.
  *
  * @param buf  Data received. Type of data in buffer (wifi_promiscuous_pkt_t) indicated by 'type' parameter.
  * @param type  promiscuous packet type.
  *
  */
typedef void (* wifi_promiscuous_cb_t)(void *buf, int len, wifi_promiscuous_pkt_type_t type);

typedef struct {
    wifi_promiscuous_filter_t filter;
    wifi_promiscuous_cb_t promisc_cb;
} wifi_promiscuous_cfg_t;

/**
  * @brief Register the RX callback function in the promiscuous mode.
  *
  * Each time a packet is received, the registered callback function will be called.
  *
  * @param cb  callback
  *
  * @return
  *    - TRS_OK: succeed
  *    - TRS_ERR_WIFI_NOT_INIT: WiFi is not initialized
  */
int wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb);

/**
  * @brief     Enable the promiscuous mode.
  *
  * @param     en  false - disable, true - enable
  *
  * @return
  *    - TRS_OK: succeed
  *    - TRS_ERR_WIFI_NOT_INIT: WiFi is not initialized
  */
int wifi_set_promiscuous(bool en);

/**
  * @brief     Get the promiscuous mode.
  *
  * @param[out] en  store the current status of promiscuous mode
  *
  * @return
  *    - TRS_OK: succeed
  *    - TRS_ERR_WIFI_NOT_INIT: WiFi is not initialized
  *    - TRS_ERR_INVALID_ARG: invalid argument
  */
int wifi_get_promiscuous(bool *en);

/**
  * @brief Enable the promiscuous mode packet type filter.
  *
  * @note The default filter is to filter all packets except WIFI_PKT_MISC
  *
  * @param filter the packet type filtered in promiscuous mode.
  *
  * @return
  *    - TRS_OK: succeed
  *    - TRS_ERR_WIFI_NOT_INIT: WiFi is not initialized
  */
int wifi_set_promiscuous_filter(wifi_promiscuous_filter_t *filter);

/**
  * @brief     Get the promiscuous filter.
  *
  * @param[out] filter  store the current status of promiscuous filter
  *
  * @return
  *    - TRS_OK: succeed
  *    - TRS_ERR_WIFI_NOT_INIT: WiFi is not initialized
  *    - TRS_ERR_INVALID_ARG: invalid argument
  */
int wifi_get_promiscuous_filter(wifi_promiscuous_filter_t *filter);


#ifdef __cplusplus
}
#endif

#endif /*_WIFI_SNIFFER_H_*/
