#include "wifi_sniffer.h"
#include "system_wifi.h"
#include "smartconfig.h"
#include "smartconfig_notify.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "zconfig_ieee80211.h"
#include "awss_aplist.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "psm_system.h"

#define PASSWORD_MAX_LEN    64
#define SSID_MAX_LEN        32
#define MAX_CHANNELS 13
#define CHANNEL_TIME_FREQ 200   //500ms
#define CHANNEL_LOCK_TIME 3000   //15000ms
#define DEBUG 0
#define SC_TASK_PRIORITY 5

extern uint16_t g_ssidIndex;
typedef struct sc_head
{
    unsigned char dummyap[26];
    unsigned char dummy[32];
} sc_head_t;


#define USR_DATA_BUFF_MAX_SIZE    (PASSWORD_MAX_LEN + 1 + SSID_MAX_LEN)
typedef enum
{
    SC_STATE_STOPED = 0,
    SC_STATE_IDLE,
    SC_STATE_SRC_LOCKED,
    SC_STATE_MAGIC_CODE_COMPLETE,
    SC_STATE_PREFIX_CODE_COMPLETE,
    SC_STATE_COMPLETE
} SC_STATE;

#define MAX_PTYPE           5
#define MULTI_TYPE_TODS     0
#define MULTI_TYPE_FRDS     1
#define BRODA_TYPE_TODS     2
#define BRODA_TYPE_FRDS     3
#define UF_TYPE_NULL        4
#define MAX_GUIDE_RECORD    4
typedef struct
{
    unsigned short  length_record[MAX_PTYPE][MAX_GUIDE_RECORD + 1];
}guide_code_record;

#define MAX_MAGIC_CODE_RECORD    5
typedef struct
{
    unsigned short record[MAX_PTYPE][MAX_MAGIC_CODE_RECORD + 1];
}magic_code_record;

#define MAX_PREFIX_CODE_RECORD    4
typedef struct
{
    unsigned short record[MAX_PTYPE][MAX_PREFIX_CODE_RECORD + 1];
}prefix_code_record;

#define MAX_SEQ_CODE_RECORD    6
typedef struct
{
    unsigned short record[MAX_PTYPE][MAX_SEQ_CODE_RECORD + 1];
}seq_code_record;

typedef struct {
        guide_code_record guide_code;
        magic_code_record magic_code;
        prefix_code_record prefix_code;
        seq_code_record  seq_code;
}sc_data;

typedef struct
{
    char pwd[ZC_MAX_PASSWD_LEN];                        
    char ssid[ZC_MAX_SSID_LEN];
    unsigned char pswd_len;
    unsigned char ssid_len;
    unsigned char random_num;
    unsigned char ssid_crc;
    unsigned char usr_data[USR_DATA_BUFF_MAX_SIZE];
    SC_STATE sc_state;
    unsigned char src_mac[6];
    unsigned char need_seq;
    unsigned char base_len[MAX_PTYPE];
    unsigned char total_len;
    unsigned char pswd_lencrc;
    unsigned char recv_len;
    unsigned short seq_success_map;
    unsigned short seq_success_map_cmp;
    sc_data data;
}sc_local_context;

typedef struct _sc_config
{
    sc_callback_t callback;
    unsigned int timeout_s;
    TimerHandle_t channel_timer;
    // TimerHandle_t lock_chn_timer;
    TimerHandle_t task_timer;
}sc_config;

const char sc_vers[] = "V1.0";
static sc_head_t sc_head = {{0},{0}};
static sc_local_context *sc_context;
static sc_config sc_cf;
static TaskHandle_t sc_task_handle = NULL;
static QueueHandle_t g_event_queue = NULL;
static sc_result_t sc_result;

int g_current_channel;
int g_round;
int g_hiddenSsid;
int g_mutilFlag;
unsigned char g_mutilAddr[6] = {0};

const char* smartconfig_version(void)
{
    return sc_vers;
}

static int sc_memcmp(unsigned char *str1, unsigned char *str2, size_t n)
{
    if(str1 == NULL || str2 == NULL || n == 0){
        return -1;
    }
	const unsigned char *p1 = str1, *p2 = str2;

    if (n == 0)
            return 0;

    while (*p1 == *p2) {
            p1++;
            p2++;
            n--;
            if (n == 0)
                return 0;
    }

    return *p1 - *p2;
}

static void resest_sc_data()
{
    memset(&sc_context->data, 0, sizeof(sc_data));
}

static int sc_get_result(sc_result_t* result)
{
    memcpy(result, sc_context, sizeof(sc_result_t));
    sc_debug("sc_get_result result ssid %s\n",result->ssid);

    return 0;
}
static int sc_change_channel(void)
{
    memset(&sc_head, 0, sizeof(sc_head_t));
    resest_sc_data();
    return 0;
}


static int sn_check(uint16_t sn, int type)
{
    static uint16_t last_sn[MAX_PTYPE];
    // sc_log("type %d last sn %d\n",type,last_sn[type] );
    if(sn != last_sn[type] || sn == 0){
        last_sn[type] = sn;
        return 1;
    } else {
        return 0;
    }
}

static int find_ap_in_aplist(const uint8_t *ap_mac, struct ap_info *t_ap)
{
    struct ap_info *target_ap = zconfig_get_apinfo(ap_mac);

    if (target_ap != NULL) {
        sc_info("target ap ssid %s channel %d\n", target_ap->ssid, target_ap->channel);
        os_timer_stop(sc_cf.channel_timer);
        sc_change_channel();
        sc_context->sc_state = SC_STATE_SRC_LOCKED;
        memcpy(t_ap,target_ap,sizeof(struct ap_info));
    }

    return 0;
}

/* return 1 is valid 
 * return 0 is invalid
 * */
int sc_filter(const unsigned char *frame, int size, int *type)
{
    int isvalid = 0;
    int i;
    unsigned char ch;
    static bool qos_flag = false;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)frame;
    unsigned char mutil_addr[6] = {0x01,0x00,0x5e,0x00,0x00,0x00};
    unsigned char broad_addr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    // after discover
    if(size < 24) {
        goto invalid;
    } else if (frame == NULL) {
        isvalid = 1;
        *type = UF_TYPE_NULL;
        goto invalid;
    }

    for (i = 0; i < 24; i++) {
        ch = *((unsigned char*)frame + i);
        sc_head.dummy[i] = ch;
    }

    // retrasmition
    if ((sc_head.dummy[1] & 0x08) != 0x00) {
        // sc_log("retran sn %d \n",(sc_head.dummy[22]+sc_head.dummy[23]*256)>>4);
        goto invalid;
    }

    //protected
    /*if ((sc_head.dummy[1] & 0x40) == 0x00) {
        // sc_log("open sn %d \n",(sc_head.dummy[22]+sc_head.dummy[23]*256)>>4);
        goto invalid;
    }*/
    //deretion
    if (ieee80211_hdrlen(hdr->frame_control) == 26){
        qos_flag = true;
    } else {
        qos_flag = false;
    }

    memset(g_mutilAddr, 0, sizeof(g_mutilAddr));
    if((sc_head.dummy[1] & 0x02) == 0x02){
        //from ds = 1
        // sc_log("sn %d len %d\n",(sc_head.dummy[22]+sc_head.dummy[23]*256)>>4,size);
        // system_printf("1 qos_flag %d addr1 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[4],sc_head.dummy[5],sc_head.dummy[6],sc_head.dummy[7],sc_head.dummy[8],sc_head.dummy[9]);
        // system_printf("1 qos_flag %d addr2 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[10],sc_head.dummy[11],sc_head.dummy[12],sc_head.dummy[13],sc_head.dummy[14],sc_head.dummy[15]);
        // system_printf("1 qos_flag %d addr3 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[16],sc_head.dummy[17],sc_head.dummy[18],sc_head.dummy[19],sc_head.dummy[20],sc_head.dummy[21]);
        if (qos_flag == true) {
            if (sc_memcmp(mutil_addr,&sc_head.dummy[4], 3) == 0 && sc_head.dummy[7] != 0x7f) {
                *type = MULTI_TYPE_FRDS;
                memcpy(g_mutilAddr, &sc_head.dummy[4], 6);
            } else if (sc_memcmp(broad_addr,&sc_head.dummy[4], 6) == 0) {
                *type = BRODA_TYPE_FRDS;
            } else {
                goto invalid;
            }
        } else {
            if (sc_memcmp(mutil_addr,&sc_head.dummy[4], 3) == 0 && sc_head.dummy[7] != 0x7f) {
                *type = MULTI_TYPE_FRDS;
                memcpy(g_mutilAddr, &sc_head.dummy[4], 6);
            } else if (sc_memcmp(broad_addr,&sc_head.dummy[4], 6) == 0) {
                *type = BRODA_TYPE_FRDS;
            } else {
                goto invalid;
            }
        }
    } else if ((sc_head.dummy[1] & 0x01) == 0x01) {
        // system_printf("2 qos_flag %d addr1 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[4],sc_head.dummy[5],sc_head.dummy[6],sc_head.dummy[7],sc_head.dummy[8],sc_head.dummy[9]);
        // system_printf("2 qos_flag %d addr2 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[10],sc_head.dummy[11],sc_head.dummy[12],sc_head.dummy[13],sc_head.dummy[14],sc_head.dummy[15]);
        // system_printf("2 qos_flag %d addr3 %02x %02x %02x %02x %02x %02x\n",qos_flag, sc_head.dummy[16],sc_head.dummy[17],sc_head.dummy[18],sc_head.dummy[19],sc_head.dummy[20],sc_head.dummy[21]);
        if (qos_flag == true) {
            if(sc_memcmp(mutil_addr,&sc_head.dummy[16], 3) == 0 && sc_head.dummy[19] != 0x7f) {
                *type = MULTI_TYPE_TODS;
                memcpy(g_mutilAddr, &sc_head.dummy[16], 6);
            } else if (sc_memcmp(broad_addr,&sc_head.dummy[16], 6) == 0) {
                *type = BRODA_TYPE_TODS;
            } else {
                goto invalid;
            }
        } else {
            if (sc_memcmp(mutil_addr,&sc_head.dummy[16], 3) == 0 && sc_head.dummy[19] != 0x7f) {
                    *type = MULTI_TYPE_TODS;
                    memcpy(g_mutilAddr, &sc_head.dummy[16], 6);
                } else if(sc_memcmp(broad_addr,&sc_head.dummy[16],6) == 0) {
                    *type = BRODA_TYPE_TODS;
                } else {
                goto invalid;
            }
        }
    } 
    int ret = sn_check((sc_head.dummy[22]+sc_head.dummy[23]*256)>>4, *type);
    if(ret == 0){
        goto invalid;
    }
    // //Address 1
    // for(i=4; i<10; i++)
    //     if(sc_head.dummy[i]!=sc_head.dummyap[i])
    //         goto invalid;
    // //Address 2
    // for(i=10; i<16; i++)
    //     if(sc_head.dummy[i]!=sc_head.dummyap[i])
    //         goto invalid;
    // //Address 3
    // for(i=16; i<22; i++)
    //     if(sc_head.dummy[i]!=sc_head.dummyap[i])
    //         goto invalid;

    isvalid = 1;
    if (*type == MAX_PTYPE) {
        isvalid = 0;
    }

invalid:
    return isvalid;
}

/* return 1 is valid 
 * return 0 is invalid
 * */
int sc_discover_filter(const unsigned char *frame, int size, int type)
{
    int isvalid = 0;

    if (size >= 1024 || type == MULTI_TYPE_FRDS || type == MULTI_TYPE_TODS) {
        isvalid = 1;
    }

    return isvalid;
}

static void sc_record_move_ones(void *base_addr, int record_num)
{
    int i; 
    unsigned short *record_base = base_addr;

    for(i = 0; i < record_num; i++)
    {
        if(record_base[i+1] != record_base[i])
            record_base[i] = record_base[i+1];
    }
}

static void sc_add_seq_data(const unsigned char *data, int seq)
{
    if(seq < sc_context->need_seq)
    {
        if((seq*4 + 4) <= USR_DATA_BUFF_MAX_SIZE)
        {
            if((sc_context->seq_success_map & (1 << seq)) == 0) 
            {
                memcpy(sc_context->usr_data + seq*4, data, 4);
                sc_context->seq_success_map |= (1 << seq);
            }
        }
    }
}
static void swap(uint16_t *p1,uint16_t *p2){
    uint16_t temp = *p1;
    *p1 = *p2;
    *p2 = temp;
}

static bool append_prefix(void *base_addr, unsigned short length, int type)
{
    int i; 
    unsigned short *record_base = base_addr;
    record_base[MAX_GUIDE_RECORD] = length;
    //if duplicate
    for(i = 0; i < 4; i++){
        if(length == record_base[i])
            goto check;           
    }
    for(i = 3; i >= 0; i--)
    {
        if(record_base[i] == 0)
            swap(&record_base[i+1],&record_base[i]);
        else if(record_base[i+1] < record_base[i])
            swap(&record_base[i],&record_base[i+1]);
        else{
            break;
        }
    }
    
check:
    sc_debug("type ----------- %d length %d\n",type,length );
    for(i = 0; i <= 4; i++)
    {
        sc_debug("%d ",record_base[i]);
    }
    sc_debug("\n");
    for(i = 0; i < 4; i++)
    {
        if(record_base[i] == 0){
            return false;
        }
    }
    sc_info("prefix code receive success, type=%d, start length=%d\n", type, record_base[0]);
    return true;
}

static void clear_record(void *base_addr)
{
    int i; 
    unsigned short *record_base = base_addr;
    for(i = 0; i < 4; i++)
    {
        record_base[i] = 0;
    }
}

static int get_base_len(const uint8_t* frame, unsigned short length, int type)
{
    struct ap_info target_ap = {0};
    if(length > 1080 && length < 1120 && sc_context->base_len[type] == 0) {
        sc_context->data.guide_code.length_record[type][MAX_GUIDE_RECORD] = length;
    } else {
        return 0;
    }
    bool ret = append_prefix(sc_context->data.guide_code.length_record[type],length, type);
    // sc_record_move_ones(sc_context->data.guide_code.length_record[type], MAX_GUIDE_RECORD);
    if(ret == true){
        if((sc_context->data.guide_code.length_record[type][1] - sc_context->data.guide_code.length_record[type][0] == 1) &&
        (sc_context->data.guide_code.length_record[type][2] - sc_context->data.guide_code.length_record[type][1] == 1) &&
        (sc_context->data.guide_code.length_record[type][3] - sc_context->data.guide_code.length_record[type][2] == 1))
        {
            sc_context->base_len[type] = sc_context->data.guide_code.length_record[type][0] - 1024;
            sc_info("base len:%d\n", sc_context->base_len[type]);
            if(type == MULTI_TYPE_FRDS || type == BRODA_TYPE_FRDS){
                find_ap_in_aplist(&frame[10],&target_ap);
            } else if (type == UF_TYPE_NULL) {
                g_current_channel = wifi_rf_get_channel();
                return 1;
            } else {
                find_ap_in_aplist(&frame[4],&target_ap);
            }
            if(target_ap.ssid[0] == 0){
                sc_info("target_ap.ssid = NULL !!!\n");
                clear_record(sc_context->data.guide_code.length_record[type]);
                g_hiddenSsid = 1;
                g_current_channel = target_ap.channel;
                return 1;
            }
            sc_context->ssid_len = strlen(target_ap.ssid);
            memcpy(sc_context->ssid, target_ap.ssid, sc_context->ssid_len);
            g_current_channel = target_ap.channel;
            // system_printf("sc_context->data.guide_code.length_record[type+1][1] %d\n",sc_context->data.guide_code.length_record[type+1][1]);
            return 1;
        }
        clear_record(sc_context->data.guide_code.length_record[type]);
        return 0;
    } 

	return 0;
}

static int multi_lock_chan(const uint8_t *frame, int type);

static void sc_recv_discover(const uint8_t* frame, unsigned short length, int type)
{
    int success;

    if (type == MULTI_TYPE_FRDS || type == MULTI_TYPE_TODS) {
        success = multi_lock_chan(frame, type);
        if (success) {
            g_mutilFlag = 1;
        }
    } else {
        success = get_base_len(frame, length, type);
    }

    if (success) {
        sc_context->sc_state = SC_STATE_SRC_LOCKED;
        resest_sc_data();
        sc_info("sc_recv_discover success\n");
    }
}


static void sc_process_magic_code(int type, unsigned short length)
{
    // system_printf("length %d type %d base %d\n",length,type,sc_context->base_len[type]);
    if(sc_context->base_len[type] == 0){
        return;
    }

    sc_context->data.magic_code.record[type][MAX_MAGIC_CODE_RECORD] = length - sc_context->base_len[type];

    sc_record_move_ones(sc_context->data.magic_code.record[type], MAX_MAGIC_CODE_RECORD);

    if(((sc_context->data.magic_code.record[type][0]&0x01f0)==0x0040)&&
        ((sc_context->data.magic_code.record[type][1]&0x01f0)==0x0010)&&
        ((sc_context->data.magic_code.record[type][2]&0x01f0)==0x0020)&&
        ((sc_context->data.magic_code.record[type][3]&0x01f0)==0x0030)) {
        if((sc_context->data.magic_code.record[type][0] + 
            sc_context->data.magic_code.record[type][1] + 
            sc_context->data.magic_code.record[type][2] +
            sc_context->data.magic_code.record[type][3]) == sc_context->data.magic_code.record[type][4]) {
            sc_context->total_len = ((sc_context->data.magic_code.record[type][0] & 0x000F) << 4) + (sc_context->data.magic_code.record[type][1] & 0x000F);
            if(sc_context->total_len > 128) {
                sc_context->total_len -= 128;
            }  else if (sc_context->total_len == 1) {
                sc_info("total_len:%d, AP is open mode !\n", sc_context->total_len);
                sc_context->sc_state = SC_STATE_COMPLETE;
                resest_sc_data();
                return;
            }
            sc_context->ssid_crc = ((sc_context->data.magic_code.record[type][2] & 0x000F) << 4) + (sc_context->data.magic_code.record[type][3] & 0x000F);
            //TODO:double check magic code
            sc_context->sc_state = SC_STATE_MAGIC_CODE_COMPLETE;
            resest_sc_data();
            sc_info("sc_process_magic_code success\n");
            sc_info("total_len:%d, ssid crc:%x\n", sc_context->total_len, sc_context->ssid_crc);
        }
    }
}

static void sc_process_prefix_code(int type, unsigned short length)
{
    if(sc_context->base_len[type] == 0){
        return;
    }
    sc_context->data.prefix_code.record[type][MAX_PREFIX_CODE_RECORD] = length - sc_context->base_len[type];

    sc_record_move_ones(sc_context->data.prefix_code.record[type], MAX_PREFIX_CODE_RECORD );

    if((sc_context->data.prefix_code.record[type][0]&0x01f0)==0x0040&&
        (sc_context->data.prefix_code.record[type][1]&0x01f0)==0x0050&&
            (sc_context->data.prefix_code.record[type][2]&0x01f0)==0x0060&&
            (sc_context->data.prefix_code.record[type][3]&0x01f0)==0x0070)
    {
        sc_context->pswd_len = ((sc_context->data.prefix_code.record[type][0] & 0x000F) << 4) + (sc_context->data.prefix_code.record[type][1] & 0x000F);
        if(sc_context->pswd_len > PASSWORD_MAX_LEN) {
            sc_context->pswd_len = 0;
        } 
        sc_context->pswd_lencrc = ((sc_context->data.prefix_code.record[type][2] & 0x000F) << 4) + (sc_context->data.prefix_code.record[type][3] & 0x000F);
        if(calcrc_1byte(sc_context->pswd_len)==sc_context->pswd_lencrc)
        {
            sc_context->sc_state = SC_STATE_PREFIX_CODE_COMPLETE;
        }
        else
        {
            sc_debug("password length crc error.\n");
            resest_sc_data();
            return;
        }

        // only receive password, and random
        sc_context->need_seq = (sc_context->total_len + 3)/4; 
        sc_context->seq_success_map_cmp = (1 << sc_context->need_seq) - 1; // EXAMPLE: need_seq = 5; seq_success_map_cmp = 0x1f; 
        sc_info("len %d need_seq %d sc_context->total_len %d\n", sc_context->ssid_len, sc_context->need_seq,sc_context->total_len);
            
        resest_sc_data();
        sc_info("sc_add_seq_data success\n");
        sc_info("pswd_len:%d, pswd_lencrc:%x, need seq:%d, seq map:%x\n", 
                sc_context->pswd_len, sc_context->pswd_lencrc, sc_context->need_seq, sc_context->seq_success_map_cmp);
    }
}

static int sc_pkt_seq_type(int type) 
{
    if(((sc_context->data.seq_code.record[type][0]&0x180)==0x80) &&
        ((sc_context->data.seq_code.record[type][1]&0x180)==0x80) && 
        ((sc_context->data.seq_code.record[type][2]&0x0100)==0x0100) && 
        ((sc_context->data.seq_code.record[type][3]&0x0100)==0x0100) && 
        ((sc_context->data.seq_code.record[type][4]&0x0100)==0x0100) && 
        ((sc_context->data.seq_code.record[type][5]&0x0100)==0x0100) && 
        ((sc_context->data.seq_code.record[type][1]&0x7F) <= ((sc_context->total_len>>2)+1)))
    {
        //middle seq
        return 0;
    } else if (((sc_context->data.seq_code.record[type][0]&0x180)==0x80) &&
        ((sc_context->data.seq_code.record[type][1]&0x7f)==(sc_context->need_seq - 1)) && 
        ((sc_context->data.seq_code.record[type][2]&0x0100)==0x0100)){
        //last seq
        return 1;
    } else {
        return 2;
    }
}

static void sc_process_sequence(int type,unsigned short length)
{
    uint8_t num;
    if(sc_context->base_len[type] == 0){
        return;
    }
    sc_context->data.seq_code.record[type][MAX_SEQ_CODE_RECORD] = length - sc_context->base_len[type];

    sc_record_move_ones(sc_context->data.seq_code.record[type], MAX_SEQ_CODE_RECORD);
    int pkt_type = sc_pkt_seq_type(type);
    if(pkt_type != 2)
    {
        unsigned char tempBuffer[MAX_PTYPE][6] = {0};
        tempBuffer[type][0]=sc_context->data.seq_code.record[type][0]&0x7F; //seq crc
        tempBuffer[type][1]=sc_context->data.seq_code.record[type][1]&0x7F; //seq index
        tempBuffer[type][2]=sc_context->data.seq_code.record[type][2]&0xFF; //data, same as following
        tempBuffer[type][3]=sc_context->data.seq_code.record[type][3]&0xFF;
        tempBuffer[type][4]=sc_context->data.seq_code.record[type][4]&0xFF;
        tempBuffer[type][5]=sc_context->data.seq_code.record[type][5]&0xFF;

        sc_debug("[crc:%02x][index:%d]:%02x,%02x,%02x,%02x; ",
                tempBuffer[type][0], tempBuffer[type][1],
                tempBuffer[type][2], tempBuffer[type][3], tempBuffer[type][4], tempBuffer[type][5]);
        if(pkt_type == 0 && tempBuffer[type][0] ==  (calcrc_bytes(&tempBuffer[type][1],5)&0x7F))
        {
            int cur_seq = tempBuffer[type][1];
            sc_debug("cur_seq %d\n",cur_seq);
            sc_add_seq_data(&tempBuffer[type][2], cur_seq);

            sc_debug("type %d sc_context->seq_success_map_cmp %x seq mapped:%x\n",type, sc_context->seq_success_map_cmp, sc_context->seq_success_map);
            resest_sc_data();
        } else if(pkt_type == 1 && tempBuffer[type][0] == (calcrc_bytes(&tempBuffer[type][1],sc_context->total_len%4 + 1)&0x7F)){
            int cur_seq = tempBuffer[type][1];
            sc_debug("cur_seq %d\n",cur_seq);
            sc_add_seq_data(&tempBuffer[type][2], cur_seq);

            sc_debug("sc_context->seq_success_map_cmp %x seq mapped:%x\n",sc_context->seq_success_map_cmp, sc_context->seq_success_map);
            resest_sc_data();
        } else
        {
            sc_debug("crc check error. calc crc:[%02x != %02x]\n",
                    tempBuffer[type][0],
                    calcrc_bytes(&tempBuffer[type][1],5));
        }
        if(sc_context->seq_success_map_cmp == sc_context->seq_success_map)
        {
            int i;
            sc_debug("User data is :");
            for(i=0;i<sc_context->total_len; i++) { //sc_context->pswd_len + 1
                sc_debug("%02x ",sc_context->usr_data[i]);
            }
            sc_debug("\n");
            if (type == UF_TYPE_NULL) {
                for (num = 0; num < zconfig_aplist_num; num++) {
                    if (zconfig_aplist[num].ssidCrc == sc_context->ssid_crc) {
                        memcpy(sc_context->ssid, zconfig_aplist[num].ssid, strlen(zconfig_aplist[num].ssid));
                        break;
                    }
                }
            } else {
                if ((zconfig_aplist[g_ssidIndex].ssidCrc != sc_context->ssid_crc) && g_ssidIndex > 0) {
                    memset(sc_context->ssid, 0, ZC_MAX_SSID_LEN);
                    for (num = 0; num < zconfig_aplist_num; num++) {
                        if (zconfig_aplist[num].ssidCrc == sc_context->ssid_crc) {
                            memcpy(sc_context->ssid, zconfig_aplist[num].ssid, strlen(zconfig_aplist[num].ssid));
                            break;
                        }
                    }
                    if (num == zconfig_aplist_num) {
                        g_hiddenSsid = 1;
                    }
                }
            }
            if (g_hiddenSsid == 1) {
                g_hiddenSsid = 0;
                sc_context->ssid_len = sc_context->total_len - sc_context->pswd_len - 1;
                memcpy(sc_context->ssid, &sc_context->usr_data[sc_context->pswd_len + 1], sc_context->ssid_len);
            } else {
                if ((sc_context->total_len - sc_context->pswd_len - 1) != 0) {
                    sc_context->ssid_len = sc_context->total_len - sc_context->pswd_len - 1;
                    memset(sc_context->ssid, 0, ZC_MAX_SSID_LEN);
                    memcpy(sc_context->ssid, &sc_context->usr_data[sc_context->pswd_len + 1], sc_context->ssid_len);
                    sc_info("connecting A config B\n");
                } else {
                    sc_context->ssid_len = strlen(sc_context->ssid);
                }
            }
            sc_info("ssid len %d\n",sc_context->ssid_len);
            sc_info("connect ssid: %s\n",sc_context->ssid);
            sc_context->random_num = sc_context->usr_data[sc_context->pswd_len];
            memcpy(sc_context->pwd, sc_context->usr_data, sc_context->pswd_len);
            sc_context->usr_data[sc_context->pswd_len + 1] = 0;
            sc_context->sc_state = SC_STATE_COMPLETE;
        }
    } 
}

static int multi_lock_chan(const uint8_t *frame, int type)
{
    struct ap_info target_ap = {0};
    unsigned char mutil_lock_addr[6] = {0x01,0x00,0x5e,0x7e,0x00,0x00};

    if (sc_memcmp(mutil_lock_addr, g_mutilAddr, 4) == 0) {
        if(type == MULTI_TYPE_FRDS){
            find_ap_in_aplist(&frame[10], &target_ap);
        } else if (type == UF_TYPE_NULL) {
            g_current_channel = wifi_rf_get_channel();
            return 1;
        } else {
            find_ap_in_aplist(&frame[4], &target_ap);
        }

        g_current_channel = target_ap.channel;
        if (target_ap.ssid[0] == 0) {
            sc_info("target_ap.ssid = NULL !!!\n");
            g_hiddenSsid = 1;
            return 1;
        }
        sc_context->ssid_len = strlen(target_ap.ssid);
        memcpy(sc_context->ssid, target_ap.ssid, sc_context->ssid_len);
//        sc_log("sc_context->ssid == %s====\n", sc_context->ssid);

        return 1;
    }

    return 0;
}

static void multi_control_code(const uint8_t *frame)
{
    unsigned char mutil_control_addr1[6] = {0x01,0x00,0x5e,0x71,0x00,0x00};
    unsigned char mutil_control_addr2[6] = {0x01,0x00,0x5e,0x72,0x00,0x00};
    unsigned char total_or = 0;
    static unsigned char multiControl[4] = {0};
    static int controlFlag[2] = {0};
    int num;

    if (sc_memcmp(mutil_control_addr1, g_mutilAddr, 4) == 0) {
        sc_context->total_len = g_mutilAddr[4];
        sc_context->pswd_len = g_mutilAddr[5];
        multiControl[1] = g_mutilAddr[4];
        multiControl[0] = g_mutilAddr[5];
        controlFlag[0] = 1;
        sc_debug("sc_context->total_len %d===sc_context->pswd_len=%d\n", sc_context->total_len, sc_context->pswd_len);
    } else if (sc_memcmp(mutil_control_addr2, g_mutilAddr, 4) == 0) {
        sc_context->ssid_crc = g_mutilAddr[4];
        total_or = g_mutilAddr[5];
        multiControl[2] = g_mutilAddr[4];
        multiControl[3] = g_mutilAddr[5];
        controlFlag[1] = 1;
        sc_debug("sc_context->ssid_crc %x===total_crc.=%x===\n", sc_context->ssid_crc, total_or);
        if ((zconfig_aplist[g_ssidIndex].ssidCrc != sc_context->ssid_crc) && g_ssidIndex > 0) {
            memset(sc_context->ssid, 0, ZC_MAX_SSID_LEN);
            for (num = 0; num < zconfig_aplist_num; num++) {
                if (zconfig_aplist[num].ssidCrc == sc_context->ssid_crc) {
                    memcpy(sc_context->ssid, zconfig_aplist[num].ssid, strlen(zconfig_aplist[num].ssid));
                    break;
                }
            }
            if (num == zconfig_aplist_num) {
                g_hiddenSsid = 1;
            }
        }
        sc_debug("sc_context->ssid == %s ====\n", sc_context->ssid);
    }

    if (controlFlag[0] == 1 && controlFlag[1] == 1) {
        controlFlag[0] = 0;
        controlFlag[1] = 0;
        sc_debug("multiControl ==%x==%x==%x==%x=\n", multiControl[0], multiControl[1], multiControl[2], multiControl[3]);
        total_or = multiControl[0] ^ multiControl[1] ^ multiControl[2];
        sc_debug("total_or=%x===\n", total_or);
        if (total_or == multiControl[3]) {
            if (sc_context->pswd_len > PASSWORD_MAX_LEN) {
                sc_context->pswd_len = 0;
            }
            if (sc_context->pswd_len == 0) {
                sc_info("total_len:%d, AP is open mode !\n", sc_context->total_len);
                if (sc_context->total_len == 2) {
                    sc_context->sc_state = SC_STATE_COMPLETE;
                    return;
                }
            }
            sc_context->sc_state = SC_STATE_PREFIX_CODE_COMPLETE;
        }
    }
}

static void multi_data_code(const uint8_t *frame)
{
    unsigned int numOfGroup;
    unsigned int dataNo[2] = {0};
    unsigned int dataCrc;
    static unsigned int oldDataIndex = 0;
    static unsigned int dataCount = 0;

    numOfGroup = sc_context->total_len / 2 + sc_context->total_len % 2;
    if (g_mutilAddr[3] >= 0x01 && g_mutilAddr[3] <= numOfGroup) {
        dataNo[0] = 2 * g_mutilAddr[3] - 2;
        dataNo[1] = 2 * g_mutilAddr[3] - 1;
        sc_context->usr_data[dataNo[0]] = g_mutilAddr[5];
        sc_context->usr_data[dataNo[1]] = g_mutilAddr[4];
        if (oldDataIndex != g_mutilAddr[3]) {
            dataCount++;
            oldDataIndex = g_mutilAddr[3];
        }
    } else {
        /* dataCrc = 0 && sc_context->usr_data[sc_context->total_len - 1] = 0 */
        return;
    }

    if (dataCount > numOfGroup) {
        dataCrc = calcrc_bytes(sc_context->usr_data, sc_context->total_len - 1);
        sc_debug("sc_context->usr_data[crc]=%x=====dataCrc=%x====\n", sc_context->usr_data[sc_context->total_len - 1], dataCrc);
        if (dataCrc == sc_context->usr_data[sc_context->total_len - 1]) {
            memcpy(sc_context->pwd, sc_context->usr_data, sc_context->pswd_len);
            sc_context->random_num = sc_context->usr_data[sc_context->pswd_len];
            if (g_hiddenSsid == 1) {
                g_hiddenSsid = 0;
                sc_context->ssid_len = sc_context->total_len - sc_context->pswd_len - 2;
                memcpy(sc_context->ssid, &sc_context->usr_data[sc_context->pswd_len + 1], sc_context->ssid_len);
                sc_info("ssid : %s\n", sc_context->ssid);
            }
            sc_debug("sc_context->pwd[ %s ]==sc_context->random_num=%x===\n", sc_context->pwd, sc_context->random_num);
            dataCount = 0;
            oldDataIndex = 0;
            sc_context->sc_state = SC_STATE_COMPLETE;
        }
    }
}

int sc_recv(const uint8_t * frame, int length)
{
    int type = MAX_PTYPE;

    if(!sc_filter(frame, length, &type)) {
        return SC_STATUS_CONTINUE;
    }

    switch (sc_context->sc_state) {
        case SC_STATE_IDLE:
            if(sc_discover_filter(frame, length, type)) {
                sc_recv_discover(frame, length, type);
                if(sc_context->sc_state == SC_STATE_SRC_LOCKED)
                    return SC_STATUS_LOCK_CHANNEL;
            }
            break;
        case SC_STATE_SRC_LOCKED:
            if (g_mutilFlag) {
                multi_control_code(frame);
            } else {
                get_base_len(frame, length, type);
                sc_process_magic_code(type, length);
            }
            break;
        case SC_STATE_MAGIC_CODE_COMPLETE:
            get_base_len(frame, length, type);
            sc_process_prefix_code(type, length);
            break;
        case SC_STATE_PREFIX_CODE_COMPLETE:
            if (g_mutilFlag) {
                multi_data_code(frame);
            } else {
                get_base_len(frame, length, type);
                sc_process_sequence(type, length);
            }
            break;
        case SC_STATE_STOPED:
            break;
        default:
            sc_context->sc_state = SC_STATE_IDLE;
            break;
    }

    if(sc_context->sc_state == SC_STATE_COMPLETE) {
        return SC_STATUS_GOT_SSID_PSWD;
    }

    return SC_STATUS_CONTINUE;
}


static int process_data(void *packet, int size)
{
    // todo: add mutex
    int ret;
    int msg;
    ret = sc_recv((const uint8_t *)packet, size);
    if(ret == SC_STATUS_CONTINUE)
    {
        //pass
    }
    else if(ret == SC_STATUS_LOCK_CHANNEL)
    {
        msg = SC_STATUS_LOCK_CHANNEL;
        g_round = 0;
        os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);
    }
    else if(ret == SC_STATUS_GOT_SSID_PSWD)
    {
        os_timer_stop(sc_cf.task_timer);
        sc_debug("smartconfig got ssid&pwd.");
        sc_get_result(&sc_result);
        sc_debug("Result:\nssid_crc:[%x]\nkey_len:[%d]\nkey:[%s]\nrandom:[0x%02x]\nssid:[%s]", 
            sc_result.reserved,
            sc_result.pwd_length,
            sc_result.pwd,
            sc_result.random,
            sc_result.ssid);
        msg = SC_STATUS_GOT_SSID_PSWD;
        os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);

    }
    //todo : add mutex

    return ret;
}
static int process_null(void *packet, int size)
{
    // todo: add mutex

    int ret;
    int msg;
    ret = sc_recv((const uint8_t *)packet, size);
    if(ret == SC_STATUS_CONTINUE)
    {
        //pass
    }
    else if(ret == SC_STATUS_LOCK_CHANNEL)
    {
        os_timer_stop(sc_cf.channel_timer);
        sc_debug("stop sc_cf.channel_timer success.");
        msg = SC_STATUS_LOCK_CHANNEL;
        g_round = 0;
        os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);
    }
    else if(ret == SC_STATUS_GOT_SSID_PSWD)
    {
        os_timer_stop(sc_cf.task_timer);
        sc_debug("smartconfig got ssid&pwd.");
        sc_get_result(&sc_result);
        sc_debug("Result:\nssid_crc:[%x]\nkey_len:[%d]\nkey:[%s]\nrandom:[0x%02x]\nssid:[%s]", 
            sc_result.reserved,
            sc_result.pwd_length,
            sc_result.pwd,
            sc_result.random,
            sc_result.ssid);
        msg = SC_STATUS_GOT_SSID_PSWD;
        os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);

    }
    //todo : add mutex

    return ret;
}

#define ZC_MAX_SSID_LEN     (32 + 1)/* ssid: 32 octets at most, include the NULL-terminated */
#define ZC_MAX_PASSWD_LEN   (64 + 1)/* 8-63 ascii */

int process_manager(void *packet, int size, int8_t rssi)
{
    uint8_t ssid[ZC_MAX_SSID_LEN] = {0}, bssid[ETH_ALEN] = {0};
    uint8_t auth, pairwise_cipher, group_cipher;
    struct ieee80211_hdr *hdr;
    int fc, ret, channel;
    struct ieee80211_hdr *mgmt_header = (struct ieee80211_hdr *)packet;

    if (mgmt_header == NULL) {
        return -1;
    }

    hdr = (struct ieee80211_hdr *)mgmt_header;
    fc = hdr->frame_control;

    /* just for save ap in aplist for ssid amend */
    if (!ieee80211_is_beacon(fc) && !ieee80211_is_probe_resp(fc)) {
        return -1;
    }

    ret = ieee80211_get_bssid((uint8_t *)mgmt_header, bssid);
    if (ret < 0) {
        return -1;
    }

    ret = ieee80211_get_ssid((uint8_t *)mgmt_header, size, ssid);
    if (ret < 0) {
        return -1;
    }

    channel = cfg80211_get_bss_channel((uint8_t *)mgmt_header, size);
    rssi = rssi > 0 ? rssi - 256 : rssi;

    cfg80211_get_cipher_info((uint8_t *)mgmt_header, size, &auth, &pairwise_cipher, &group_cipher);

    awss_save_apinfo(ssid, bssid, channel, auth, pairwise_cipher, group_cipher, rssi);

    return -1;
}

static void wifiSnifferCallback(void *buf, int len, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf; 

    if (type == WIFI_PKT_MGMT) {
        process_manager((char *)pkt->payload, len, pkt->rx_ctrl.rssi);
    } else if (type == WIFI_PKT_DATA) {
        process_data((char *)pkt->payload, len);
    } else if (type == WIFI_PKT_NULL) {
        process_null(NULL, len);
    }
    
}
extern int fhost_cntrl_map_check(uint8_t vif_type);
void sc_task(void* parameter)
{
    int event;

    memset(sc_context , 0, sizeof(sc_local_context));
    sc_context->sc_state = SC_STATE_IDLE;
    g_ssidIndex = 0;
    g_event_queue = os_queue_create("event_queue", 3, sizeof(int), 0);

    while(!fhost_cntrl_map_check(0));

    awss_init_ieee80211_aplist();
    wifi_promiscuous_filter_t filter = {0};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT;
    wifi_set_promiscuous_filter(&filter);
    wifi_set_promiscuous_rx_cb(wifiSnifferCallback);
    wifi_set_promiscuous(true);

    os_timer_start(sc_cf.channel_timer);
    os_timer_start(sc_cf.task_timer);
    while(1){
        if (os_queue_receive(g_event_queue, (char *)&event, sizeof(event), (portTickType)portMAX_DELAY) == 0) {
            switch (event)
            {
                case SC_STATUS_LOCK_CHANNEL:
                    if(sc_cf.callback != NULL){
                        sc_cf.callback(SC_STATUS_LOCK_CHANNEL,&g_current_channel);
                    }
                    wifi_rf_set_channel(g_current_channel);
                    memset(&filter, 0, sizeof(wifi_promiscuous_filter_t));
                    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
                    wifi_set_promiscuous_filter(&filter);
                    break;
                case SC_STATUS_GOT_SSID_PSWD:
                    if(sc_cf.callback != NULL){
                        sc_cf.callback(SC_STATUS_GOT_SSID_PSWD,&sc_result);
                    }
                    break;
                case SC_STATUS_TIMEOUT:
                    sc_debug("SC_STATUS_TIMEOUT\n");
                    sc_context->sc_state = SC_STATE_STOPED;
                    wifi_set_promiscuous(false);
                    memset(&sc_head, 0, sizeof(sc_head_t));
                    resest_sc_data();
                    awss_clear_aplist();
                    g_round = 0;
                    if(sc_cf.callback != NULL){
                        sc_cf.callback(SC_STATUS_TIMEOUT,NULL);
                    }
                    goto stop;
                case SC_STATUS_STOP:
                    sc_debug("SC_STATUS_STOP\n");
                    sc_context->sc_state = SC_STATE_STOPED;
                    wifi_set_promiscuous(false);
                    memset(&sc_head, 0, sizeof(sc_head_t));
                    resest_sc_data();
                    awss_clear_aplist();
                    g_round = 0;
                    if(sc_cf.callback != NULL){
                        sc_cf.callback(SC_STATUS_STOP,NULL);
                    }
                    goto stop;
            }
        }
    }
stop:
    if(g_event_queue) {
        os_queue_destory(g_event_queue);
    }   
    g_event_queue = NULL;
    os_task_delete(0);
}

// void sc_channel_lock_func(void* param)
// {
//     if(sc_context->sc_state != SC_STATE_COMPLETE){
//         sc_log("lock the channel but can't get result,clear the record and start again\n");
//         memset(&sc_head, 0, sizeof(sc_head_t));
//         memset(&sc_context , 0, sizeof(sc_local_context));
//         sc_context->sc_state = SC_STATE_IDLE;
//         xTimerStart(sc_cf.channel_timer, 0);
//     }
// }

bool channel_is_valid(int channel)
{
    return zconfig_check_ap_channel(channel);
}

void sc_channel_switch_func(void* param)
{
    uint8_t chn;

    if (sc_context->sc_state >= SC_STATE_SRC_LOCKED) {
        return;
    }
    chn = wifi_rf_get_channel();
    chn++;
    if(chn > MAX_CHANNELS) {
        chn = 1;
    }
    while(g_round >= 20 && channel_is_valid(chn) == false){
        chn++;
    }
    sc_debug("sc set channel %d===g_round==%d==\n", chn, g_round);
    g_round++;
    g_current_channel = chn;
    sc_change_channel();
    wifi_rf_set_channel(chn);
    if(chn == 1 || chn == 6 || chn == 11){
        os_timer_changeperiod(sc_cf.channel_timer, 200);
    } else {
        os_timer_changeperiod(sc_cf.channel_timer, 150);
    }
}

void sc_timeout_func(void* param)
{
    int msg;
    if(sc_context->sc_state < SC_STATE_COMPLETE && sc_context->sc_state != SC_STATE_STOPED) {
        sc_error("smartconfig timeout\n");
        os_timer_delete(sc_cf.channel_timer);
        os_timer_delete(sc_cf.task_timer);
        // xTimerDelete(sc_cf.lock_chn_timer,0);
        msg = SC_STATUS_TIMEOUT;
        if(g_event_queue) {
            os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);
        }
    } else {
        return;
    }
}


int smartconfig_start(sc_callback_t cb)
{
    if (!sc_context)
    {
        sc_context = os_malloc(sizeof(sc_local_context));
        os_memset(sc_context, 0, sizeof(sc_local_context));
    }

    if (sc_context->sc_state == SC_STATE_STOPED) {
		psm_set_device_status(PSM_DEVICE_SMART_CONFIG, PSM_DEVICE_STATUS_ACTIVE);
        wifi_stop_station();
        g_mutilFlag = 0;
        g_hiddenSsid = 0;
        memset(sc_context , 0, sizeof(sc_local_context));
        memset(&sc_cf , 0, sizeof(sc_config));
        sc_context->sc_state = SC_STATE_IDLE;
        sc_cf.callback = cb;
        if(sc_cf.timeout_s == 0){
            sc_cf.timeout_s = 120;//default expired time is 120s
        }
        sc_info("smart config start...\n version %s\n",sc_vers);
        sc_cf.channel_timer = os_timer_create("ChannelTimer",CHANNEL_TIME_FREQ, pdFALSE, sc_channel_switch_func, NULL);
        // sc_cf.lock_chn_timer = xTimerCreate("LockchannelTimer",CHANNEL_LOCK_TIME, pdFALSE, NULL, sc_channel_lock_func);
        sc_cf.task_timer = os_timer_create("TaskTimer", sc_cf.timeout_s * 1000, pdFALSE, sc_timeout_func, NULL);
        sc_task_handle = (TaskHandle_t)os_task_create((const char *)"smart_config_task", SC_TASK_PRIORITY,
            1024 * sizeof(StackType_t), (task_entry_t)sc_task, NULL);

        if (sc_task_handle == NULL || sc_cf.channel_timer == NULL || sc_cf.task_timer == NULL) {
            if (sc_task_handle) {
                os_task_delete((int)sc_task_handle);
            }
            if (sc_cf.channel_timer) {
                os_timer_delete(sc_cf.channel_timer);
            }
            if (sc_cf.task_timer) {
                os_timer_delete(sc_cf.task_timer);
            }
        }
        return 0;
    } else {
        sc_error("smartconfig is already started,can't start it twice\n");
        return -1;
    }
}


int smartconfig_stop(void)
{
    int msg;
    int result = 0;

    if (!sc_context)
    {
        return -1;
    }

    if (sc_context->sc_state != SC_STATE_STOPED) {
        sc_info("smartconfig stop\n");
        if (sc_cf.channel_timer) {
            g_round = 0;
            os_timer_stop(sc_cf.channel_timer);
            os_timer_delete(sc_cf.channel_timer);
        }
        if (sc_cf.task_timer) {
            os_timer_stop(sc_cf.task_timer);
            os_timer_delete(sc_cf.task_timer);
        }
        msg = SC_STATUS_STOP;
        if (g_event_queue) {
            os_queue_send(g_event_queue, (char *)&msg, sizeof(msg), portMAX_DELAY);
        }
        awss_deinit_ieee80211_aplist();
		psm_set_device_status(PSM_DEVICE_SMART_CONFIG, PSM_DEVICE_STATUS_IDLE);
        sc_context->sc_state = SC_STATE_STOPED;
        result = 0;
    } else {
        sc_error("smartconfig is already stopped,can't stop it twice\n");
        result =  -1;
    }

    return result;
}

static sc_result_t result;
static int connect_wifi_handle = 0;
static os_timer_handle_t connect_wifi_timer;
static bool g_connectWifiTimeoutFlag = false;

/*void wifi_sta_connnect(const char *ssid, const char *pwd)
{
    wifi_remove_config_all(STATION_IF);
    wifi_add_config(STATION_IF);

    wifi_config_ssid(STATION_IF, (unsigned char *)ssid);
    if (pwd && pwd[0] != '\0') {
        wifi_set_password(STATION_IF, (char *)pwd);
    } else {
        wifi_set_password(STATION_IF, NULL);
    }

    return;
}*/

void connect_wifi_timeout_func(void* param)
{
    g_connectWifiTimeoutFlag = true;
    if (connect_wifi_timer) {
        os_timer_stop(connect_wifi_timer);
        os_timer_delete(connect_wifi_timer);
    }
}

void connect_wifi_task(void *param) 
{
    unsigned int sta_ip = 0;
    sc_notify_t msg = {0};
    wifi_config_u config={0};

    g_connectWifiTimeoutFlag = false;
    /* 创建一个30s的超时检测，30s没有获取到ip，删除connect_wifi_task */
    connect_wifi_timer = os_timer_create("ConnectWifiTimer", 30 * 1000, pdFALSE, connect_wifi_timeout_func, NULL);
    os_timer_start(connect_wifi_timer);

    memcpy(config.sta.ssid, result.ssid, result.ssid_length);
    memcpy(config.sta.password, result.pwd, result.pwd_length);
    wifi_start_station(&config);

    while (1) {
        wifi_get_ip_addr(STATION_IF, &sta_ip);
        if (sta_ip != 0) {
            os_timer_stop(connect_wifi_timer);
            os_timer_delete(connect_wifi_timer);
            msg.random = result.random;
            sc_notify_send(&msg);
            break;
        } else if (g_connectWifiTimeoutFlag) {
            sc_error("connect_wifi_timeout!\n");
            break;
        }
        os_msleep(100);
    }
    sc_debug("connect_wifi_task vTaskDelete !\n");
    connect_wifi_handle = 0;
    os_task_delete(0);
}

void sc_callback(smartconfig_status_t status, void *pdata)
{
    int lock_chn;
    if (status == SC_STATUS_LOCK_CHANNEL){
        memcpy(&lock_chn,pdata,sizeof(int));
        sc_debug("target AP is in channel %d\n",lock_chn);
    }
    else if(status == SC_STATUS_GOT_SSID_PSWD) {
         sc_debug("got ssid and password\n");
        memcpy(&result,pdata,sizeof(sc_result_t));
        sc_debug("Result:\nssid_crc:[%x]\nkey_len:[%d]\nkey:[%s]\nrandom:[0x%02x]\nssid:[%s] %d", 
            result.reserved,
            result.pwd_length,
            result.pwd,
            result.random,
            result.ssid,
            result.ssid_length);
        smartconfig_stop();

    } else if(status == SC_STATUS_STOP){
        /* optional operation: connect to AP and send notify message to cellphone */
        if (connect_wifi_handle) {
            if (connect_wifi_timer) {
                os_timer_stop(connect_wifi_timer);
                os_timer_delete(connect_wifi_timer);
            }
            os_task_delete((int)connect_wifi_handle);
            connect_wifi_handle = 0;
        }
        connect_wifi_handle = os_task_create((const char *)"connect_wifi_task", 5, 512 * sizeof(StackType_t),
            (task_entry_t)connect_wifi_task, NULL);
    } else {
        sc_debug("smartconfig status %d\n",status);
    }
}


