/*
 * wpa_supplicant - Internal definitions
 * Copyright (c) 2003-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef WPA_SUPPLICANT_I_H
#define WPA_SUPPLICANT_I_H

#include "utils/list.h"
#include "common/defs.h"
#include "common/sae.h"
#include "common/wpa_ctrl.h"
#include "wps/wps_defs.h"
#include "drivers/driver.h"
#include "config_ssid.h"

extern const char *const wpa_supplicant_version;
extern const char *const wpa_supplicant_license;
#ifndef CONFIG_NO_STDOUT_DEBUG
extern const char *const wpa_supplicant_full_license1;
extern const char *const wpa_supplicant_full_license2;
extern const char *const wpa_supplicant_full_license3;
extern const char *const wpa_supplicant_full_license4;
extern const char *const wpa_supplicant_full_license5;
#endif /* CONFIG_NO_STDOUT_DEBUG */

struct wpa_sm;
struct wpa_supplicant;
struct ibss_rsn;
struct scan_info;
struct wpa_bss;
struct wpa_scan_results;
struct hostapd_hw_modes;
struct wpa_driver_associate_params;

/*
 * Forward declarations of private structures used within the ctrl_iface
 * backends. Other parts of wpa_supplicant do not have access to data stored in
 * these structures.
 */
struct ctrl_iface_priv;
struct ctrl_iface_global_priv;
struct wpas_dbus_priv;
struct wpas_binder_priv;

/**
 * struct wpa_interface - Parameters for wpa_supplicant_add_iface()
 */
struct wpa_interface {
	/**
	 * driver - Driver interface name, or %NULL to use the default driver
	 */
	//const char *driver;

	/**
	 * driver_param - Driver interface parameters
	 *
	 * If a configuration file is not used, this variable can be used to
	 * set the driver_param parameters that would have otherwise been read
	 * from the configuration file. If both confname and driver_param are
	 * set, driver_param is used to override the value from configuration
	 * file.
	 */
	//const char *driver_param;

	/**
	 * ifname - Interface name
	 */
	const char *ifname;
};

/**
 * struct wpa_params - Parameters for wpa_supplicant_init()
 */
struct wpa_params {
#ifdef CONFIG_P2P
	/**
	 * conf_p2p_dev - Configuration file used to hold the
	 * P2P Device configuration parameters.
	 *
	 * This can also be %NULL. In such a case, if a P2P Device dedicated
	 * interfaces is created, the main configuration file will be used.
	 */
	char *conf_p2p_dev;
#endif /* CONFIG_P2P */

#ifdef CONFIG_MATCH_IFACE
	/**
	 * match_ifaces - Interface descriptions to match
	 */
	struct wpa_interface *match_ifaces;

#endif /* CONFIG_MATCH_IFACE */
};

struct p2p_srv_bonjour {
	struct dl_list list;
	struct wpabuf *query;
	struct wpabuf *resp;
};

struct p2p_srv_upnp {
	struct dl_list list;
	u8 version;
	char *service;
};

/**
 * struct wpa_global - Internal, global data for all %wpa_supplicant interfaces
 *
 * This structure is initialized by calling wpa_supplicant_init() when starting
 * %wpa_supplicant.
 */
struct wpa_global {
	struct wpa_supplicant *ifaces;
	struct ctrl_iface_global_priv *ctrl_iface;
	void **drv_priv;
	size_t drv_count;
};


/**
 * struct wpa_radio - Internal data for per-radio information
 *
 * This structure is used to share data about configured interfaces
 * (struct wpa_supplicant) that share the same physical radio, e.g., to allow
 * better coordination of offchannel operations.
 */
struct wpa_radio {
	char name[16]; /* from driver_ops get_radio_name() or empty if not
			* available */
	unsigned int external_scan_running:1;
	unsigned int num_active_works;
	struct dl_list ifaces; /* struct wpa_supplicant::radio_list entries */
	struct dl_list work; /* struct wpa_radio_work::list entries */
};

#define MAX_ACTIVE_WORKS 2


/**
 * struct wpa_radio_work - Radio work item
 */
struct wpa_radio_work {
	struct dl_list list;
	unsigned int freq; /* known frequency (MHz) or 0 for multiple/unknown */
	const char *type;
	struct wpa_supplicant *wpa_s;
	void (*cb)(struct wpa_radio_work *work, int deinit);
	void *ctx;
	unsigned int started:1;
	struct os_reltime time;
	unsigned int bands;
};

int radio_add_work(struct wpa_supplicant *wpa_s, unsigned int freq,
		   const char *type, int next,
		   void (*cb)(struct wpa_radio_work *work, int deinit),
		   void *ctx);
void radio_work_done(struct wpa_radio_work *work);
void radio_remove_works(struct wpa_supplicant *wpa_s,
			const char *type, int remove_all);
void radio_work_check_next(struct wpa_supplicant *wpa_s);
struct wpa_radio_work *
radio_work_pending(struct wpa_supplicant *wpa_s, const char *type);

struct wpa_connect_work {
	unsigned int sme:1;
	unsigned int bss_removed:1;
	struct wpa_bss *bss;
	struct wpa_ssid *ssid;
};

int wpas_valid_bss_ssid(struct wpa_supplicant *wpa_s, struct wpa_bss *test_bss,
			struct wpa_ssid *test_ssid);
void wpas_connect_work_free(struct wpa_connect_work *cwork);
void wpas_connect_work_done(struct wpa_supplicant *wpa_s);

struct wpa_external_work {
	unsigned int id;
	char type[100];
	unsigned int timeout;
};

enum wpa_radio_work_band wpas_freq_to_band(int freq);
unsigned int wpas_get_bands(struct wpa_supplicant *wpa_s, const int *freqs);

/**
 * offchannel_send_action_result - Result of offchannel send Action frame
 */
enum offchannel_send_action_result {
	OFFCHANNEL_SEND_ACTION_SUCCESS /**< Frame was send and acknowledged */,
	OFFCHANNEL_SEND_ACTION_NO_ACK /**< Frame was sent, but not acknowledged
				       */,
	OFFCHANNEL_SEND_ACTION_FAILED /**< Frame was not sent due to a failure
				       */
};

struct wps_ap_info {
	u8 bssid[ETH_ALEN];
	enum wps_ap_info_type {
		WPS_AP_NOT_SEL_REG,
		WPS_AP_SEL_REG,
		WPS_AP_SEL_REG_OUR
	} type;
	unsigned int tries;
	struct os_reltime last_attempt;
	unsigned int pbc_active;
	u8 uuid[WPS_UUID_LEN];
};

#define WPA_FREQ_USED_BY_INFRA_STATION BIT(0)
#define WPA_FREQ_USED_BY_P2P_CLIENT BIT(1)

struct wpa_used_freq_data {
	int freq;
	unsigned int flags;
};

#define RRM_NEIGHBOR_REPORT_TIMEOUT 1 /* 1 second for AP to send a report */

/*
 * struct rrm_data - Data used for managing RRM features
 */
struct rrm_data {
	/* rrm_used - indication regarding the current connection */
	unsigned int rrm_used:1;

	/*
	 * notify_neighbor_rep - Callback for notifying report requester
	 */
	void (*notify_neighbor_rep)(void *ctx, struct wpabuf *neighbor_rep);

	/*
	 * neighbor_rep_cb_ctx - Callback context
	 * Received in the callback registration, and sent to the callback
	 * function as a parameter.
	 */
	void *neighbor_rep_cb_ctx;

	/* next_neighbor_rep_token - Next request's dialog token */
	u8 next_neighbor_rep_token;
};

enum wpa_supplicant_test_failure {
	WPAS_TEST_FAILURE_NONE,
	WPAS_TEST_FAILURE_SCAN_TRIGGER,
};

struct icon_entry {
	struct dl_list list;
	u8 bssid[ETH_ALEN];
	u8 dialog_token;
	char *file_name;
	u8 *image;
	size_t image_len;
};

struct wpa_bss_tmp_disallowed {
	struct dl_list list;
	u8 bssid[ETH_ALEN];
	struct os_reltime disallowed_until;
};

typedef struct {
    u8 ssid[SSID_MAX_LEN + 1];
    u8 ssid_len;
    u8 bssid[ETH_ALEN];
    u8 bssid_set;
    u8 passive;
    u32 scan_time; /*scan time per channel.*/
    int freq;
}wpa_manual_scan_cfg_t;
/**
 * struct wpa_supplicant - Internal data for wpa_supplicant interface
 *
 * This structure contains the internal data for core wpa_supplicant code. This
 * should be only used directly from the core code. However, a pointer to this
 * data is used from other files as an arbitrary context pointer in calls to
 * core functions.
 */
struct wpa_supplicant {
	struct wpa_global *global;
	struct wpa_radio *radio; /* shared radio context */
	struct dl_list radio_list; /* list head: struct wpa_radio::ifaces */
	struct wpa_supplicant *parent;
	//struct wpa_supplicant *p2pdev;
	struct wpa_supplicant *next;
	struct l2_packet_data *l2;
	//struct l2_packet_data *l2_br;
	unsigned char own_addr[ETH_ALEN];
	char ifname[16];
#ifdef CONFIG_MATCH_IFACE
	int matched;
#endif /* CONFIG_MATCH_IFACE */
#ifdef CONFIG_CTRL_IFACE_DBUS
	char *dbus_path;
#endif /* CONFIG_CTRL_IFACE_DBUS */
#ifdef CONFIG_CTRL_IFACE_DBUS_NEW
	char *dbus_new_path;
	char *dbus_groupobj_path;
#ifdef CONFIG_AP
	char *preq_notify_peer;
#endif /* CONFIG_AP */
#endif /* CONFIG_CTRL_IFACE_DBUS_NEW */
#ifdef CONFIG_CTRL_IFACE_BINDER
	const void *binder_object_key;
#endif /* CONFIG_CTRL_IFACE_BINDER */


	struct wpa_config *conf;
	int countermeasures;
	struct os_reltime last_michael_mic_error;
	u8 bssid[ETH_ALEN];
	u8 pending_bssid[ETH_ALEN]; /* If wpa_state == WPA_ASSOCIATING, this
				     * field contains the target BSSID. */
	//int reassociate; /* reassociation requested */
	//unsigned int reassoc_same_bss:1; /* reassociating to the same BSS */
	//unsigned int reassoc_same_ess:1; /* reassociating to the same ESS */
	int disconnected; /* all connections disabled; i.e., do no reassociate
			   * before this has been cleared */
	struct wpa_ssid *current_ssid;
	//struct wpa_ssid *last_ssid;
	struct wpa_bss *current_bss;
	int ap_ies_from_associnfo;
	unsigned int assoc_freq;

	/* Selected configuration (based on Beacon/ProbeResp WPA IE) */
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int wpa_proto;
	int mgmt_group_cipher;

	void *drv_priv; /* private data used by driver_ops */
	void *global_drv_priv;

	//enum set_band setband;

	/* Preferred network for the next connection attempt */
	//struct wpa_ssid *next_ssid;

	/* previous scan was wildcard when interleaving between
	 * wildcard scans and specific SSID scan when max_ssids=1 */
	//int prev_scan_wildcard;
	/*struct wpa_ssid *prev_scan_ssid;*/ /* previously scanned SSID;
					  * NULL = not yet initialized (start
					  * with wildcard SSID)
					  * WILDCARD_SSID_SCAN = wildcard
					  * SSID was used in the previous scan
					  */
#define WILDCARD_SSID_SCAN ((struct wpa_ssid *) 1)

	//struct wpa_ssid *prev_sched_ssid; /* last SSID used in sched scan */
	//int sched_scan_timeout;
	//int first_sched_scan;
	//int sched_scan_timed_out;
	//struct sched_scan_plan *sched_scan_plans;
	//size_t sched_scan_plans_num;

    wpa_manual_scan_cfg_t *manual_scan_cfg;

	struct dl_list bss; /* struct wpa_bss::list */
	struct dl_list bss_id; /* struct wpa_bss::list_id */
	size_t num_bss;
	unsigned int bss_update_idx;
	unsigned int bss_next_id;

	 /*
	  * Pointers to BSS entries in the order they were in the last scan
	  * results.
	  */
	struct wpa_bss **last_scan_res;
	unsigned int last_scan_res_used;
	unsigned int last_scan_res_size;
	struct os_reltime last_scan;

	const struct wpa_driver_ops *driver;
	//int interface_removed; /* whether the network interface has been
	//			* removed */
	struct wpa_sm *wpa;

	struct ctrl_iface_priv *ctrl_iface;

	enum wpa_states wpa_state;
	struct wpa_radio_work *scan_work;
	int scanning;
	//int sched_scanning;
	//unsigned int sched_scan_stop_req:1;
	int new_connection;

	int eapol_received; /* number of EAPOL packets received after the
			     * previous association event */

	//char imsi[20];
	//int mnc_len;

	//unsigned char last_eapol_src[ETH_ALEN];

	unsigned int keys_cleared; /* bitfield of key indexes that the driver is
				    * known not to be configured with a key */

	/**
	 * extra_blacklist_count - Sum of blacklist counts after last connection
	 *
	 * This variable is used to maintain a count of temporary blacklisting
	 * failures (maximum number for any BSS) over blacklist clear
	 * operations. This is needed for figuring out whether there has been
	 * failures prior to the last blacklist clear operation which happens
	 * whenever no other not-blacklisted BSS candidates are available. This
	 * gets cleared whenever a connection has been established successfully.
	 */
	//int extra_blacklist_count;

	/**
	 * scan_req - Type of the scan request
	 */
	enum scan_req_type {
		/**
		 * NORMAL_SCAN_REQ - Normal scan request
		 *
		 * This is used for scans initiated by wpa_supplicant to find an
		 * AP for a connection.
		 */
		NORMAL_SCAN_REQ,

		/**
		 * INITIAL_SCAN_REQ - Initial scan request
		 *
		 * This is used for the first scan on an interface to force at
		 * least one scan to be run even if the configuration does not
		 * include any enabled networks.
		 */
		INITIAL_SCAN_REQ,

		/**
		 * MANUAL_SCAN_REQ - Manual scan request
		 *
		 * This is used for scans where the user request a scan or
		 * a specific wpa_supplicant operation (e.g., WPS) requires scan
		 * to be run.
		 */
		MANUAL_SCAN_REQ
	} scan_req, last_scan_req;
	enum wpa_states scan_prev_wpa_state;
	//struct os_reltime scan_trigger_time, scan_start_time;
	/* Minimum freshness requirement for connection purposes */
	struct os_reltime scan_min_time;
	//int scan_runs; /* number of scan runs since WPS was started */
	//int *next_scan_freqs;
	//int *manual_scan_freqs;
	//int *manual_sched_scan_freqs;
	//unsigned int manual_scan_passive:1;
	//unsigned int manual_scan_use_id:1;
	//unsigned int manual_scan_only_new:1;
	unsigned int own_scan_requested:1;
	unsigned int own_scan_running:1;
	unsigned int clear_driver_scan_cache:1;
	//unsigned int manual_scan_id;
	int scan_interval; /* time in sec between scans to find suitable AP */
	int normal_scans; /* normal scans run before sched_scan */
	int scan_for_connection; /* whether the scan request was triggered for
				  * finding a connection */
#define MAX_SCAN_ID 16
	//int scan_id[MAX_SCAN_ID];
	//unsigned int scan_id_count;
	//u8 next_scan_bssid[ETH_ALEN];

	//struct wpa_ssid_value *ssids_from_scan_req;
	//unsigned int num_ssids_from_scan_req;

	u64 drv_flags;
	unsigned int drv_enc;
	//unsigned int drv_smps_modes;

	/*
	 * A bitmap of supported protocols for probe response offload. See
	 * struct wpa_driver_capa in driver.h
	 */
	unsigned int probe_resp_offloads;

	/* extended capabilities supported by the driver */
	//const u8 *extended_capa, *extended_capa_mask;
	//unsigned int extended_capa_len;

	int max_scan_ssids;
	//int max_sched_scan_ssids;
	//unsigned int max_sched_scan_plans;
	//unsigned int max_sched_scan_plan_interval;
	//unsigned int max_sched_scan_plan_iterations;
	//int sched_scan_supported;
	//unsigned int max_match_sets;
	//unsigned int max_remain_on_chan;
	unsigned int max_stations;

	int pending_mic_error_report;
	int pending_mic_error_pairwise;
	int mic_errors_seen; /* Michael MIC errors with the current PTK */

	//struct wps_context *wps;
	//int wps_success; /* WPS success event received */
	//struct wps_er *wps_er;
	//unsigned int wps_run;
	//struct os_reltime wps_pin_start_time;
	//int blacklist_cleared;

	struct wpabuf *pending_eapol_rx;
	struct os_reltime pending_eapol_rx_time;
	u8 pending_eapol_rx_src[ETH_ALEN];
	unsigned int last_eapol_matches_bssid:1;
	unsigned int eap_expected_failure:1;
	unsigned int reattach:1; /* reassociation to the same BSS requested */
	unsigned int added_vif:1;
	unsigned int wnmsleep_used:1;

	//struct os_reltime last_mac_addr_change;
	//int last_mac_addr_style;

	//struct ibss_rsn *ibss_rsn;

	//int set_sta_uapsd;
	//int sta_uapsd;
	//int set_ap_uapsd;
	//int ap_uapsd;

#ifdef CONFIG_SME
	struct {
		u8 ssid[SSID_MAX_LEN];
		size_t ssid_len;
		int freq;
		u8 assoc_req_ie[200];
		size_t assoc_req_ie_len;
		int mfp;
		int ft_used;
		u8 mobility_domain[2];
		u8 *ft_ies;
		size_t ft_ies_len;
		u8 prev_bssid[ETH_ALEN];
		int prev_bssid_set;
		int auth_alg;
		int proto;

		int sa_query_count; /* number of pending SA Query requests;
				     * 0 = no SA Query in progress */
		int sa_query_timed_out;
		u8 *sa_query_trans_id; /* buffer of WLAN_SA_QUERY_TR_ID_LEN *
					* sa_query_count octets of pending
					* SA Query transaction identifiers */
		struct os_reltime sa_query_start;
		struct os_reltime last_unprot_disconnect;
		enum { HT_SEC_CHAN_UNKNOWN,
		       HT_SEC_CHAN_ABOVE,
		       HT_SEC_CHAN_BELOW } ht_sec_chan;
		u8 sched_obss_scan;
		u16 obss_scan_int;
		u16 bss_max_idle_period;
#ifdef CONFIG_SAE
		struct sae_data sae;
		struct wpabuf *sae_token;
		int sae_group_index;
		unsigned int sae_pmksa_caching:1;
		u16 seq_num;
		u8 ext_auth_bssid[ETH_ALEN];
		u8 ext_auth_ssid[SSID_MAX_LEN];
		size_t ext_auth_ssid_len;
#endif /* CONFIG_SAE */
	} sme;
#endif /* CONFIG_SME */

#ifdef CONFIG_AP
	struct hostapd_iface *ap_iface;
	//void (*ap_configured_cb)(void *ctx, void *data);
	//void *ap_configured_cb_ctx;
	//void *ap_configured_cb_data;
#endif /* CONFIG_AP */

	//struct hostapd_iface *ifmsh;
#ifdef CONFIG_MESH
	struct mesh_rsn *mesh_rsn;
	int mesh_if_idx;
	unsigned int mesh_if_created:1;
	unsigned int mesh_ht_enabled:1;
	unsigned int mesh_vht_enabled:1;
#endif /* CONFIG_MESH */

	//unsigned int off_channel_freq;
	//struct wpabuf *pending_action_tx;
	//u8 pending_action_src[ETH_ALEN];
	//u8 pending_action_dst[ETH_ALEN];
	//u8 pending_action_bssid[ETH_ALEN];
	//unsigned int pending_action_freq;
	//int pending_action_no_cck;
	//int pending_action_without_roc;
	//unsigned int pending_action_tx_done:1;
	//void (*pending_action_tx_status_cb)(struct wpa_supplicant *wpa_s,
	//				    unsigned int freq, const u8 *dst,
	//				    const u8 *src, const u8 *bssid,
	//				    const u8 *data, size_t data_len,
	//				    enum offchannel_send_action_result
	//				    result);
	//unsigned int roc_waiting_drv_freq;
	//int action_tx_wait_time;

	//int p2p_mgmt;

#ifdef CONFIG_P2P
	struct p2p_go_neg_results *go_params;
	int create_p2p_iface;
	u8 pending_interface_addr[ETH_ALEN];
	char pending_interface_name[100];
	int pending_interface_type;
	int p2p_group_idx;
	unsigned int pending_listen_freq;
	unsigned int pending_listen_duration;
	enum {
		NOT_P2P_GROUP_INTERFACE,
		P2P_GROUP_INTERFACE_PENDING,
		P2P_GROUP_INTERFACE_GO,
		P2P_GROUP_INTERFACE_CLIENT
	} p2p_group_interface;
	struct p2p_group *p2p_group;
	int p2p_long_listen; /* remaining time in long Listen state in ms */
	char p2p_pin[10];
	int p2p_wps_method;
	u8 p2p_auth_invite[ETH_ALEN];
	int p2p_sd_over_ctrl_iface;
	int p2p_in_provisioning;
	int p2p_in_invitation;
	int p2p_invite_go_freq;
	int pending_invite_ssid_id;
	int show_group_started;
	u8 go_dev_addr[ETH_ALEN];
	int pending_pd_before_join;
	u8 pending_join_iface_addr[ETH_ALEN];
	u8 pending_join_dev_addr[ETH_ALEN];
	int pending_join_wps_method;
	u8 p2p_join_ssid[SSID_MAX_LEN];
	size_t p2p_join_ssid_len;
	int p2p_join_scan_count;
	int auto_pd_scan_retry;
	int force_long_sd;
	u16 pending_pd_config_methods;
	enum {
		NORMAL_PD, AUTO_PD_GO_NEG, AUTO_PD_JOIN, AUTO_PD_ASP
	} pending_pd_use;

	/*
	 * Whether cross connection is disallowed by the AP to which this
	 * interface is associated (only valid if there is an association).
	 */
	int cross_connect_disallowed;

	/*
	 * Whether this P2P group is configured to use cross connection (only
	 * valid if this is P2P GO interface). The actual cross connect packet
	 * forwarding may not be configured depending on the uplink status.
	 */
	int cross_connect_enabled;

	/* Whether cross connection forwarding is in use at the moment. */
	int cross_connect_in_use;

	/*
	 * Uplink interface name for cross connection
	 */
	char cross_connect_uplink[100];

	unsigned int p2p_auto_join:1;
	unsigned int p2p_auto_pd:1;
	unsigned int p2p_persistent_group:1;
	unsigned int p2p_fallback_to_go_neg:1;
	unsigned int p2p_pd_before_go_neg:1;
	unsigned int p2p_go_ht40:1;
	unsigned int p2p_go_vht:1;
	unsigned int user_initiated_pd:1;
	unsigned int p2p_go_group_formation_completed:1;
	unsigned int group_formation_reported:1;
	unsigned int waiting_presence_resp;
	int p2p_first_connection_timeout;
	unsigned int p2p_nfc_tag_enabled:1;
	unsigned int p2p_peer_oob_pk_hash_known:1;
	unsigned int p2p_disable_ip_addr_req:1;
	unsigned int p2ps_method_config_any:1;
	unsigned int p2p_cli_probe:1;
	int p2p_persistent_go_freq;
	int p2p_persistent_id;
	int p2p_go_intent;
	int p2p_connect_freq;
	struct os_reltime p2p_auto_started;
	struct wpa_ssid *p2p_last_4way_hs_fail;
	struct wpa_radio_work *p2p_scan_work;
	struct wpa_radio_work *p2p_listen_work;
	struct wpa_radio_work *p2p_send_action_work;

	u16 p2p_oob_dev_pw_id; /* OOB Device Password Id for group formation */
	struct wpabuf *p2p_oob_dev_pw; /* OOB Device Password for group
					* formation */
	u8 p2p_peer_oob_pubkey_hash[WPS_OOB_PUBKEY_HASH_LEN];
	u8 p2p_ip_addr_info[3 * 4];

	/* group common frequencies */
	int *p2p_group_common_freqs;
	unsigned int p2p_group_common_freqs_num;
	u8 p2ps_join_addr[ETH_ALEN];

	unsigned int p2p_go_max_oper_chwidth;
	unsigned int p2p_go_vht_center_freq2;
	int p2p_lo_started;
#endif /* CONFIG_P2P */

#ifdef CONFIG_BGSCAN
	struct wpa_ssid *bgscan_ssid;
	const struct bgscan_ops *bgscan;
	void *bgscan_priv;
#endif    

	struct wpa_ssid *connect_without_scan;

	//struct wps_ap_info *wps_ap;
	//size_t num_wps_ap;
	//int wps_ap_iter;

	//int after_wps;
	//int known_wps_freq;
	//unsigned int wps_freq;
	//int wps_fragment_size;
	//int auto_reconnect_disabled;

	 /* Channel preferences for AP/P2P GO use */
	//int best_24_freq;
	//int best_5_freq;
	//int best_overall_freq;

	//struct gas_query *gas;

#ifdef CONFIG_INTERWORKING
	unsigned int fetch_anqp_in_progress:1;
	unsigned int network_select:1;
	unsigned int auto_select:1;
	unsigned int auto_network_select:1;
	unsigned int interworking_fast_assoc_tried:1;
	unsigned int fetch_all_anqp:1;
	unsigned int fetch_osu_info:1;
	unsigned int fetch_osu_waiting_scan:1;
	unsigned int fetch_osu_icon_in_progress:1;
	struct wpa_bss *interworking_gas_bss;
	unsigned int osu_icon_id;
	struct dl_list icon_head; /* struct icon_entry */
	struct osu_provider *osu_prov;
	size_t osu_prov_count;
	struct os_reltime osu_icon_fetch_start;
	unsigned int num_osu_scans;
	unsigned int num_prov_found;
#endif /* CONFIG_INTERWORKING */
	unsigned int drv_capa_known;

	struct {
		struct hostapd_hw_modes *modes;
		u16 num_modes;
		u16 flags;
	} hw;
	enum local_hw_capab {
		CAPAB_NO_HT_VHT,
		CAPAB_HT,
		CAPAB_HT40,
		CAPAB_VHT,
	} hw_capab;
#ifdef CONFIG_MACSEC
	struct ieee802_1x_kay *kay;
#endif /* CONFIG_MACSEC */

	/* WLAN_REASON_* reason codes. Negative if locally generated. */
	int disconnect_reason;

	/* WLAN_STATUS_* status codes from (Re)Association Response frame. */
	u16 assoc_status_code;

	//struct ext_password_data *ext_pw;

	//struct wpabuf *last_gas_resp, *prev_gas_resp;
	//u8 last_gas_addr[ETH_ALEN], prev_gas_addr[ETH_ALEN];
	//u8 last_gas_dialog_token, prev_gas_dialog_token;

	unsigned int no_keep_alive:1;
	unsigned int ext_mgmt_frame_handling:1;
	unsigned int ext_eapol_frame_io:1;
	//unsigned int wmm_ac_supported:1;
	unsigned int ext_work_in_progress:1;
	unsigned int own_disconnect_req:1;

#ifdef CONFIG_WNM
	u8 wnm_dialog_token;
	u8 wnm_reply;
	u8 wnm_num_neighbor_report;
	u8 wnm_mode;
	u16 wnm_dissoc_timer;
	u8 wnm_bss_termination_duration[12];
	struct neighbor_report *wnm_neighbor_report_elements;
	struct os_reltime wnm_cand_valid_until;
	u8 wnm_cand_from_bss[ETH_ALEN];
#endif /* CONFIG_WNM */

#ifdef CONFIG_TESTING_GET_GTK
	u8 last_gtk[32];
	size_t last_gtk_len;
#endif /* CONFIG_TESTING_GET_GTK */

	//unsigned int num_multichan_concurrent;
	struct wpa_radio_work *connect_work;

	unsigned int ext_work_id;

	//struct wpabuf *vendor_elem[NUM_VENDOR_ELEM_FRAMES];

#ifdef CONFIG_TESTING_OPTIONS
	struct l2_packet_data *l2_test;
	unsigned int extra_roc_dur;
	enum wpa_supplicant_test_failure test_failure;
	unsigned int reject_btm_req_reason;
	unsigned int p2p_go_csa_on_inv:1;
	unsigned int ignore_auth_resp:1;
	unsigned int ignore_assoc_disallow:1;
#endif /* CONFIG_TESTING_OPTIONS */

	//struct wmm_ac_assoc_data *wmm_ac_assoc_info;
	//struct wmm_tspec_element *tspecs[WMM_AC_NUM][TS_DIR_IDX_COUNT];
	//struct wmm_ac_addts_request *addts_request;
	//u8 wmm_ac_last_dialog_token;
	//struct wmm_tspec_element *last_tspecs;
	//u8 last_tspecs_count;

	//struct rrm_data rrm;

#ifdef CONFIG_FST
	struct fst_iface *fst;
	const struct wpabuf *fst_ies;
	struct wpabuf *received_mb_ies;
#endif /* CONFIG_FST */

#ifdef CONFIG_MBO
	/* Multiband operation non-preferred channel */
	struct wpa_mbo_non_pref_channel {
		enum mbo_non_pref_chan_reason reason;
		u8 oper_class;
		u8 chan;
		u8 preference;
	} *non_pref_chan;
	size_t non_pref_chan_num;
	u8 mbo_wnm_token;
#endif /* CONFIG_MBO */

	/*
	 * This should be under CONFIG_MBO, but it is left out to allow using
	 * the bss_temp_disallowed list for other purposes as well.
	 */
	//struct dl_list bss_tmp_disallowed;

	/*
	 * Content of a measurement report element with type 8 (LCI),
	 * own location.
	 */
	//struct wpabuf *lci;
	//struct os_reltime lci_time;
};


/* wpa_supplicant.c */
void wpa_supplicant_apply_ht_overrides(
	struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
	struct wpa_driver_associate_params *params);
void wpa_supplicant_apply_vht_overrides(
	struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid,
	struct wpa_driver_associate_params *params);

int wpa_set_wep_keys(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);
int wpa_supplicant_set_wpa_none_key(struct wpa_supplicant *wpa_s,
				    struct wpa_ssid *ssid);

int wpa_supplicant_reload_configuration(struct wpa_supplicant *wpa_s);

const char * wpa_supplicant_state_txt(enum wpa_states state);
int wpa_supplicant_update_mac_addr(struct wpa_supplicant *wpa_s);
int wpa_supplicant_driver_init(struct wpa_supplicant *wpa_s);
int wpa_supplicant_set_suites(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss, struct wpa_ssid *ssid,
			      u8 *wpa_ie, size_t *wpa_ie_len);
void wpa_supplicant_associate(struct wpa_supplicant *wpa_s,
			      struct wpa_bss *bss,
			      struct wpa_ssid *ssid);
void wpa_supplicant_set_non_wpa_policy(struct wpa_supplicant *wpa_s,
				       struct wpa_ssid *ssid);
void wpa_supplicant_initiate_eapol(struct wpa_supplicant *wpa_s);
void wpa_clear_keys(struct wpa_supplicant *wpa_s, const u8 *addr);
void wpa_supplicant_req_auth_timeout(struct wpa_supplicant *wpa_s,
				     int sec, int usec);
void wpa_supplicant_reinit_autoscan(struct wpa_supplicant *wpa_s);
void wpa_supplicant_set_state(struct wpa_supplicant *wpa_s,
			      enum wpa_states state);
struct wpa_ssid * wpa_supplicant_get_ssid(struct wpa_supplicant *wpa_s);
const char * wpa_supplicant_get_eap_mode(struct wpa_supplicant *wpa_s);
void wpa_supplicant_cancel_auth_timeout(struct wpa_supplicant *wpa_s);
void wpa_supplicant_deauthenticate(struct wpa_supplicant *wpa_s,
				   int reason_code);

struct wpa_ssid * wpa_supplicant_add_network(struct wpa_supplicant *wpa_s);
int wpa_supplicant_remove_network(struct wpa_supplicant *wpa_s, int id);
void wpa_supplicant_enable_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid);
void wpa_supplicant_disable_network(struct wpa_supplicant *wpa_s,
				    struct wpa_ssid *ssid);
void wpa_supplicant_select_network(struct wpa_supplicant *wpa_s,
				   struct wpa_ssid *ssid);
int wpas_set_pkcs11_engine_and_module_path(struct wpa_supplicant *wpa_s,
					   const char *pkcs11_engine_path,
					   const char *pkcs11_module_path);
int wpa_supplicant_set_ap_scan(struct wpa_supplicant *wpa_s,
			       int ap_scan);
int wpa_supplicant_set_bss_expiration_age(struct wpa_supplicant *wpa_s,
					  unsigned int expire_age);
int wpa_supplicant_set_bss_expiration_count(struct wpa_supplicant *wpa_s,
					    unsigned int expire_count);
int wpa_supplicant_set_scan_interval(struct wpa_supplicant *wpa_s,
				     int scan_interval);
void free_hw_features(struct wpa_supplicant *wpa_s);

void wpa_show_license(void);

struct wpa_interface * wpa_supplicant_match_iface(struct wpa_global *global,
						  const char *ifname);
struct wpa_supplicant * wpa_supplicant_add_iface(struct wpa_global *global,
						 struct wpa_interface *iface,
						 struct wpa_supplicant *parent);
int wpa_supplicant_remove_iface(struct wpa_global *global,
				struct wpa_supplicant *wpa_s,
				int terminate);
struct wpa_supplicant * wpa_supplicant_get_iface(struct wpa_global *global,
						 const char *ifname);
struct wpa_global * wpa_supplicant_init();
int wpa_supplicant_run(struct wpa_global *global);
void wpa_supplicant_deinit(struct wpa_global *global);

int wpa_supplicant_scard_init(struct wpa_supplicant *wpa_s,
			      struct wpa_ssid *ssid);
void wpa_supplicant_terminate_proc(struct wpa_global *global);
void wpa_supplicant_rx_eapol(void *ctx, const u8 *src_addr,
			     const u8 *buf, size_t len);
void wpa_supplicant_update_config(struct wpa_supplicant *wpa_s);
void wpa_supplicant_clear_status(struct wpa_supplicant *wpa_s);
void wpas_connection_failed(struct wpa_supplicant *wpa_s, const u8 *bssid);
int wpas_driver_bss_selection(struct wpa_supplicant *wpa_s);
//void wpas_auth_failed(struct wpa_supplicant *wpa_s, char *reason);
//void wpas_clear_temp_disabled(struct wpa_supplicant *wpa_s,
//			      struct wpa_ssid *ssid, int clear_failures);
int disallowed_bssid(struct wpa_supplicant *wpa_s, const u8 *bssid);
int disallowed_ssid(struct wpa_supplicant *wpa_s, const u8 *ssid,
		    size_t ssid_len);
void wpas_request_connection(struct wpa_supplicant *wpa_s);
void wpas_request_disconnection(struct wpa_supplicant *wpa_s);
int wpas_build_ext_capab(struct wpa_supplicant *wpa_s, u8 *buf, size_t buflen);
int wpas_update_random_addr(struct wpa_supplicant *wpa_s, int style);
void add_freq(int *freqs, int *num_freqs, int freq);

void wpas_rrm_reset(struct wpa_supplicant *wpa_s);
void wpas_rrm_process_neighbor_rep(struct wpa_supplicant *wpa_s,
				   const u8 *report, size_t report_len);
int wpas_rrm_send_neighbor_rep_request(struct wpa_supplicant *wpa_s,
				       const struct wpa_ssid_value *ssid,
				       int lci, int civic,
				       void (*cb)(void *ctx,
						  struct wpabuf *neighbor_rep),
				       void *cb_ctx);
void wpas_rrm_handle_radio_measurement_request(struct wpa_supplicant *wpa_s,
					       const u8 *src,
					       const u8 *frame, size_t len);
void wpas_rrm_handle_link_measurement_request(struct wpa_supplicant *wpa_s,
					      const u8 *src,
					      const u8 *frame, size_t len,
					      int rssi);


/* MBO functions */
int wpas_mbo_ie(struct wpa_supplicant *wpa_s, u8 *buf, size_t len);
const u8 * wpas_mbo_get_bss_attr(struct wpa_bss *bss, enum mbo_attr_id attr);
int wpas_mbo_update_non_pref_chan(struct wpa_supplicant *wpa_s,
				  const char *non_pref_chan);
void wpas_mbo_scan_ie(struct wpa_supplicant *wpa_s, struct wpabuf *ie);
int wpas_mbo_supp_op_class_ie(struct wpa_supplicant *wpa_s, int freq, u8 *pos,
			      size_t len);
void wpas_mbo_ie_trans_req(struct wpa_supplicant *wpa_s, const u8 *ie,
			   size_t len);
size_t wpas_mbo_ie_bss_trans_reject(struct wpa_supplicant *wpa_s, u8 *pos,
				    size_t len,
				    enum mbo_transition_reject_reason reason);
void wpas_mbo_update_cell_capa(struct wpa_supplicant *wpa_s, u8 mbo_cell_capa);
struct wpabuf * mbo_build_anqp_buf(struct wpa_supplicant *wpa_s,
				   struct wpa_bss *bss);

/**
 * wpa_supplicant_ctrl_iface_ctrl_rsp_handle - Handle a control response
 * @wpa_s: Pointer to wpa_supplicant data
 * @ssid: Pointer to the network block the reply is for
 * @field: field the response is a reply for
 * @value: value (ie, password, etc) for @field
 * Returns: 0 on success, non-zero on error
 *
 * Helper function to handle replies to control interface requests.
 */
int wpa_supplicant_ctrl_iface_ctrl_rsp_handle(struct wpa_supplicant *wpa_s,
					      struct wpa_ssid *ssid,
					      const char *field,
					      const char *value);

void ibss_mesh_setup_freq(struct wpa_supplicant *wpa_s,
			  const struct wpa_ssid *ssid,
			  struct hostapd_freq_params *freq);

/* events.c */
void wpa_supplicant_mark_disassoc(struct wpa_supplicant *wpa_s);
int wpa_supplicant_connect(struct wpa_supplicant *wpa_s,
			   struct wpa_bss *selected,
			   struct wpa_ssid *ssid);
void wpa_supplicant_stop_countermeasures(void *eloop_ctx, void *sock_ctx);
void wpa_supplicant_delayed_mic_error_report(void *eloop_ctx, void *sock_ctx);
void wnm_bss_keep_alive_deinit(struct wpa_supplicant *wpa_s);
int wpa_supplicant_fast_associate(struct wpa_supplicant *wpa_s);
struct wpa_bss * wpa_supplicant_pick_network(struct wpa_supplicant *wpa_s,
					     struct wpa_ssid **selected_ssid);

/* eap_register.c */
int eap_register_methods(void);

/**
 * Utility method to tell if a given network is for persistent group storage
 * @ssid: Network object
 * Returns: 1 if network is a persistent group, 0 otherwise
 */
static inline int network_is_persistent_group(struct wpa_ssid *ssid)
{
	return ssid->disabled == 2;
}

int wpas_network_disabled(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);
int wpas_get_ssid_pmf(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);

int wpas_init_ext_pw(struct wpa_supplicant *wpa_s);

void dump_freq_data(struct wpa_supplicant *wpa_s, const char *title,
		    struct wpa_used_freq_data *freqs_data,
		    unsigned int len);

int get_shared_radio_freqs_data(struct wpa_supplicant *wpa_s,
				struct wpa_used_freq_data *freqs_data,
				unsigned int len);
int get_shared_radio_freqs(struct wpa_supplicant *wpa_s,
			   int *freq_array, unsigned int len);

void wpas_network_reenabled(void *eloop_ctx, void *timeout_ctx);

void wpas_vendor_elem_update(struct wpa_supplicant *wpa_s);
struct wpa_supplicant * wpas_vendor_elem(struct wpa_supplicant *wpa_s,
					 enum wpa_vendor_elem_frame frame);
int wpas_vendor_elem_remove(struct wpa_supplicant *wpa_s, int frame,
			    const u8 *elem, size_t len);

#ifdef CONFIG_FST

struct fst_wpa_obj;

void fst_wpa_supplicant_fill_iface_obj(struct wpa_supplicant *wpa_s,
				       struct fst_wpa_obj *iface_obj);

#endif /* CONFIG_FST */


struct hostapd_hw_modes * get_mode(struct hostapd_hw_modes *modes,
				   u16 num_modes, enum hostapd_hw_mode mode);

void wpa_bss_tmp_disallow(struct wpa_supplicant *wpa_s, const u8 *bssid,
			  unsigned int sec);
int wpa_is_bss_tmp_disallowed(struct wpa_supplicant *wpa_s, const u8 *bssid);

struct wpa_ssid * wpa_scan_res_match(struct wpa_supplicant *wpa_s,
				     int i, struct wpa_bss *bss,
				     struct wpa_ssid *group,
				     int only_first_ssid);

#endif /* WPA_SUPPLICANT_I_H */
