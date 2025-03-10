#include "includes.h"

#include "common.h"
#include "utils/uuid.h"
#include "utils/ip_addr.h"
#include "crypto/sha1.h"
#include "rsn_supp/wpa.h"
#include "config.h"

#include "wpas_pmk.h"

#if defined (CONFIG_NV)
#include "system_config.h"
#endif

#define WIFI_SSID_MAX_LEN   (32 + 1)
#define WIFI_PWD_MAX_LEN    (64) // pwd string support max length is 63

typedef struct {
    unsigned int sync:1;
    unsigned int pmk_set:1;
    unsigned int pmkid_set:1;
    int key_mgmt;
    char  ssid[WIFI_SSID_MAX_LEN];
    char  pwd[WIFI_PWD_MAX_LEN];
    char  bssid[6];
    char  pmk[PMK_LEN];
	char  pmkid[PMKID_LEN];
} wpas_pmk_info_t;

static   wpas_pmk_info_t wpas_pmk_info;

void wpas_pmk_info_load(void)
{    
    memset(&wpas_pmk_info, 0, sizeof(wpas_pmk_info));
    #if defined (CONFIG_NV)
    hal_system_get_config(CUSTOMER_NV_WIFI_WPAS_PMKINFO, &wpas_pmk_info, sizeof(wpas_pmk_info));
    #endif
}

void wpas_pmk_info_save(char *ssid, char *bssid, char *pwd, char *pmk, char *pmkid)
{
    if(ssid == NULL)
        return;

    strncpy(wpas_pmk_info.ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    strncpy(wpas_pmk_info.pwd, pwd, WIFI_PWD_MAX_LEN);
    memcpy(wpas_pmk_info.bssid, bssid, 6);
    memcpy(wpas_pmk_info.pmk, pmk, PMK_LEN);
    wpas_pmk_info.pmk_set = 1;
    wpas_pmk_info.sync = 1;
    wpa_printf(MSG_DEBUG, "ssid:%s pwd:%s bssid:%02x:%02x:%02x:%02x:%02x:%02x",ssid, pwd, bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);
    #if 0
    int i = 0;
    cli_printf("pmk:");
    for(i = 0; i < 32; i++)
        cli_printf("%02x ",pmk[i]&0xff);
    cli_printf("\r\n");
    #endif
    if(pmkid != NULL)
    {
        /*cli_printf("pmkid:");
        for(i = 0; i < 16; i++)
            cli_printf("%02x ",pmkid[i]&0xff);
        cli_printf("\r\n");*/
        memcpy(wpas_pmk_info.pmkid, pmkid, PMKID_LEN);
        wpas_pmk_info.pmkid_set = 1;
    }
#if defined (CONFIG_NV)
    hal_system_set_config(CUSTOMER_NV_WIFI_WPAS_PMKINFO, &wpas_pmk_info, sizeof(wpas_pmk_info));
#endif
}

int wpas_pmk_info_refresh(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid)
{
    int set= 0;
    if(ssid->ssid == NULL || ssid->passphrase == NULL)
        return -1;
    wpa_printf(MSG_DEBUG, "wpas_pmk_info.ssid=%s pwd=%s pmkid_set:%d pmk_set:%d key_mgmt=%x",wpas_pmk_info.ssid, wpas_pmk_info.pwd, wpas_pmk_info.pmkid_set, wpas_pmk_info.pmk_set, wpas_pmk_info.key_mgmt);
    wpa_printf(MSG_DEBUG, "ssid->ssid=%s ssid->passphrase=%s key_mgmt=%x",ssid->ssid, ssid->passphrase, wpa_s->key_mgmt);

    if(wpas_pmk_info.pmk_set == 1 && strcmp(wpas_pmk_info.ssid, (char *)ssid->ssid) == 0 
        && strcmp(wpas_pmk_info.pwd, ssid->passphrase) == 0 && wpas_pmk_info.key_mgmt == wpa_s->key_mgmt)
    {
        memcpy((char *)ssid->psk, wpas_pmk_info.pmk, PMK_LEN);
    }
    else if(!wpa_key_mgmt_sae(wpa_s->key_mgmt))
    {
        wpa_printf(MSG_DEBUG, "wpa/wpa2 ssid->key_mgmt");
        pbkdf2_sha1(ssid->passphrase, ssid->ssid, ssid->ssid_len, 4096, ssid->psk, PMK_LEN);
        wpas_pmk_info.pmk_set = 1;
        wpas_pmk_info.pmkid_set = 0;
        wpas_pmk_info.key_mgmt = wpa_s->key_mgmt;
        
        set = 1;
    }
    else
    {
        wpas_pmk_info.pmkid_set = 0;
        wpas_pmk_info.pmk_set = 0;
        wpas_pmk_info.key_mgmt = wpa_s->key_mgmt;
        set = 1;
    }

    ssid->psk_set = 1;
    wpa_printf(MSG_DEBUG, "ssid->psk_set = %d", ssid->psk_set);
#if defined (CONFIG_NV)
    if(set)
    {
        wpas_pmk_info_save((char *)ssid->ssid, (char *)ssid->bssid, (char *)ssid->passphrase, (char *)ssid->psk, NULL);
    }
#endif

    return 0;
}


void wpas_pmk_info_clean(void)
{
    if(wpas_pmk_info.sync == 1)
    {
        memset(&wpas_pmk_info, 0, sizeof(wpas_pmk_info));
        #if defined (CONFIG_NV)
        hal_system_set_config(CUSTOMER_NV_WIFI_WPAS_PMKINFO, &wpas_pmk_info, sizeof(wpas_pmk_info));
        #endif
    }
}

char * wpas_pmkid_get(void)
{
    wpa_printf(MSG_DEBUG, "get pmkid[%d]", wpas_pmk_info.pmkid_set);
    /*int i;
    for(i = 0; i < 16; i++)
        cli_printf("%02x ",wpas_pmk_info.pmkid[i]&0xff);
    cli_printf("\r\n");*/
    if(wpas_pmk_info.pmkid_set == 1)
        return wpas_pmk_info.pmkid;

    return NULL;
}

#if 0
#include "cli.h"

int wpas_pmk_info_clean_func(cmd_tbl_t *t, int argc, char *argv[])
{
    cli_printf("wpas_pmk_info_clean_func\r\n");
    memset(&wpas_pmk_info, 0, sizeof(wpas_pmk_info));
#if defined (CONFIG_NV)
    hal_system_set_config(CUSTOMER_NV_WIFI_WPAS_PMKINFO, &wpas_pmk_info, sizeof(wpas_pmk_info));
#endif

	return CMD_RET_SUCCESS;
}
CLI_CMD(pmk_clean, wpas_pmk_info_clean_func, "get_mac", "get_mac");
#endif

