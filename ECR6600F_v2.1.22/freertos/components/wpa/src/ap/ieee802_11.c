/*
 * hostapd / IEEE 802.11 Management
 * Copyright (c) 2002-2014, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#ifndef CONFIG_NATIVE_WINDOWS

#include "utils/common.h"
#include "utils/eloop.h"
#include "crypto/crypto.h"
#include "crypto/sha256.h"
#include "crypto/random.h"
#include "common/ieee802_11_defs.h"
#include "common/ieee802_11_common.h"
#include "common/wpa_ctrl.h"
#include "common/sae.h"
#include "wps/wps.h"
#include "hostapd.h"
#include "beacon.h"
#include "ieee802_11_auth.h"
#include "sta_info.h"
#include "wpa_auth.h"
#include "wmm.h"
#include "ap_list.h"
#include "ap_config.h"
#include "ap_mlme.h"
#include "ap_drv_ops.h"
#include "hw_features.h"
#include "ieee802_11.h"
#include "pmksa_cache_auth.h"


u8 * hostapd_eid_supp_rates(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	int i, num, count;

	if (hapd->iface->current_rates == NULL)
		return eid;

	*pos++ = WLAN_EID_SUPP_RATES;
	num = hapd->iface->num_rates;
	if (hapd->iconf->ieee80211n && hapd->iconf->require_ht)
		num++;
	if (hapd->iconf->ieee80211ac && hapd->iconf->require_vht)
		num++;
	if (num > 8) {
		/* rest of the rates are encoded in Extended supported
		 * rates element */
		num = 8;
	}

	*pos++ = num;
	for (i = 0, count = 0; i < hapd->iface->num_rates && count < num;
	     i++) {
		count++;
		*pos = hapd->iface->current_rates[i].rate / 5;
		if (hapd->iface->current_rates[i].flags & HOSTAPD_RATE_BASIC)
			*pos |= 0x80;
		pos++;
	}

	if (hapd->iconf->ieee80211n && hapd->iconf->require_ht && count < 8) {
		count++;
		*pos++ = 0x80 | BSS_MEMBERSHIP_SELECTOR_HT_PHY;
	}

	if (hapd->iconf->ieee80211ac && hapd->iconf->require_vht && count < 8) {
		count++;
		*pos++ = 0x80 | BSS_MEMBERSHIP_SELECTOR_VHT_PHY;
	}

	return pos;
}


u8 * hostapd_eid_ext_supp_rates(struct hostapd_data *hapd, u8 *eid)
{
	u8 *pos = eid;
	int i, num, count;

	if (hapd->iface->current_rates == NULL)
		return eid;

	num = hapd->iface->num_rates;
	if (hapd->iconf->ieee80211n && hapd->iconf->require_ht)
		num++;
	if (hapd->iconf->ieee80211ac && hapd->iconf->require_vht)
		num++;
	if (num <= 8)
		return eid;
	num -= 8;

	*pos++ = WLAN_EID_EXT_SUPP_RATES;
	*pos++ = num;
	for (i = 0, count = 0; i < hapd->iface->num_rates && count < num + 8;
	     i++) {
		count++;
		if (count <= 8)
			continue; /* already in SuppRates IE */
		*pos = hapd->iface->current_rates[i].rate / 5;
		if (hapd->iface->current_rates[i].flags & HOSTAPD_RATE_BASIC)
			*pos |= 0x80;
		pos++;
	}

	if (hapd->iconf->ieee80211n && hapd->iconf->require_ht) {
		count++;
		if (count > 8)
			*pos++ = 0x80 | BSS_MEMBERSHIP_SELECTOR_HT_PHY;
	}

	if (hapd->iconf->ieee80211ac && hapd->iconf->require_vht) {
		count++;
		if (count > 8)
			*pos++ = 0x80 | BSS_MEMBERSHIP_SELECTOR_VHT_PHY;
	}

	return pos;
}


u16 hostapd_own_capab_info(struct hostapd_data *hapd)
{
	int capab = WLAN_CAPABILITY_ESS;
	int privacy;
	//int dfs;
	//int i;

    #if 0
	/* Check if any of configured channels require DFS */
	dfs = hostapd_is_dfs_required(hapd->iface);
	if (dfs < 0) {
		wpa_printf_warning(MSG_WARNING, "Failed to check if DFS is required; ret=%d",
			   dfs);
		dfs = 0;
	}
    #else
    //dfs = 0;
    #endif

	if (hapd->iface->num_sta_no_short_preamble == 0 &&
	    hapd->iconf->preamble == SHORT_PREAMBLE)
		capab |= WLAN_CAPABILITY_SHORT_PREAMBLE;

	privacy = hapd->conf->ssid.wep.keys_set;

	/*if (hapd->conf->ieee802_1x &&
	    (hapd->conf->default_wep_key_len ||
	     hapd->conf->individual_wep_key_len))
		privacy = 1;*/

	if (hapd->conf->wpa)
		privacy = 1;

#ifdef CONFIG_HS20
	if (hapd->conf->osen)
		privacy = 1;
#endif /* CONFIG_HS20 */

	if (privacy)
		capab |= WLAN_CAPABILITY_PRIVACY;

	if (hapd->iface->current_mode &&
	    hapd->iface->current_mode->mode == HOSTAPD_MODE_IEEE80211G &&
	    hapd->iface->num_sta_no_short_slot_time == 0)
		capab |= WLAN_CAPABILITY_SHORT_SLOT_TIME;

	/*
	 * Currently, Spectrum Management capability bit is set when directly
	 * requested in configuration by spectrum_mgmt_required or when AP is
	 * running on DFS channel.
	 * TODO: Also consider driver support for TPC to set Spectrum Mgmt bit
	 */
	/*if (hapd->iface->current_mode &&
	    hapd->iface->current_mode->mode == HOSTAPD_MODE_IEEE80211A &&
	    (hapd->iconf->spectrum_mgmt_required || dfs))
		capab |= WLAN_CAPABILITY_SPECTRUM_MGMT;

	for (i = 0; i < RRM_CAPABILITIES_IE_LEN; i++) {
		if (hapd->conf->radio_measurements[i]) {
			capab |= IEEE80211_CAP_RRM;
			break;
		}
	}*/

	return capab;
}


#ifndef CONFIG_NO_RC4
static u16 auth_shared_key(struct hostapd_data *hapd, struct sta_info *sta,
			   u16 auth_transaction, const u8 *challenge,
			   int iswep)
{
	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG,
		       "authentication (shared key, transaction %d)",
		       auth_transaction);

	if (auth_transaction == 1) {
		if (!sta->challenge) {
			/* Generate a pseudo-random challenge */
			u8 key[8];

			sta->challenge = os_zalloc(WLAN_AUTH_CHALLENGE_LEN);
			if (sta->challenge == NULL)
				return WLAN_STATUS_UNSPECIFIED_FAILURE;

			if (os_get_random(key, sizeof(key)) < 0) {
				os_free(sta->challenge);
				sta->challenge = NULL;
				return WLAN_STATUS_UNSPECIFIED_FAILURE;
			}

			rc4_skip(key, sizeof(key), 0,
				 sta->challenge, WLAN_AUTH_CHALLENGE_LEN);
		}
		return 0;
	}

	if (auth_transaction != 3)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	/* Transaction 3 */
	if (!iswep || !sta->challenge || !challenge ||
	    os_memcmp_const(sta->challenge, challenge,
			    WLAN_AUTH_CHALLENGE_LEN)) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO,
			       "shared key authentication - invalid "
			       "challenge-response");
		return WLAN_STATUS_CHALLENGE_FAIL;
	}

	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG,
		       "authentication OK (shared key)");
	sta->flags |= WLAN_STA_AUTH;
	wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
	os_free(sta->challenge);
	sta->challenge = NULL;

	return 0;
}
#endif /* CONFIG_NO_RC4 */


static int send_auth_reply(struct hostapd_data *hapd,
			   const u8 *dst, const u8 *bssid,
			   u16 auth_alg, u16 auth_transaction, u16 resp,
			   const u8 *ies, size_t ies_len)
{
	struct ieee80211_mgmt *reply;
	u8 *buf;
	size_t rlen;
	int reply_res = WLAN_STATUS_UNSPECIFIED_FAILURE;

	rlen = IEEE80211_HDRLEN + sizeof(reply->u.auth) + ies_len;
	buf = os_zalloc(rlen);
	if (buf == NULL)
		return -1;

	reply = (struct ieee80211_mgmt *) buf;
	reply->frame_control = IEEE80211_FC(WLAN_FC_TYPE_MGMT,
					    WLAN_FC_STYPE_AUTH);
	os_memcpy(reply->da, dst, ETH_ALEN);
	os_memcpy(reply->sa, hapd->own_addr, ETH_ALEN);
	os_memcpy(reply->bssid, bssid, ETH_ALEN);

	reply->u.auth.auth_alg = host_to_le16(auth_alg);
	reply->u.auth.auth_transaction = host_to_le16(auth_transaction);
	reply->u.auth.status_code = host_to_le16(resp);

	if (ies && ies_len)
		os_memcpy(reply->u.auth.variable, ies, ies_len);

	wpa_printf(MSG_DEBUG, "authentication reply: STA=" MACSTR
		   " auth_alg=%d auth_transaction=%d resp=%d (IE len=%lu)",
		   MAC2STR(dst), auth_alg, auth_transaction,
		   resp, (unsigned long) ies_len);
	if (hostapd_drv_send_mlme(hapd, reply, rlen, 0) < 0)
		wpa_printf(MSG_INFO, "send_auth_reply: send failed");
	else
		reply_res = WLAN_STATUS_SUCCESS;

	os_free(buf);

	return reply_res;
}


#ifdef CONFIG_IEEE80211R
static void handle_auth_ft_finish(void *ctx, const u8 *dst, const u8 *bssid,
				  u16 auth_transaction, u16 status,
				  const u8 *ies, size_t ies_len)
{
	struct hostapd_data *hapd = ctx;
	struct sta_info *sta;
	int reply_res;

	reply_res = send_auth_reply(hapd, dst, bssid, WLAN_AUTH_FT,
				    auth_transaction, status, ies, ies_len);

	sta = ap_get_sta(hapd, dst);
	if (sta == NULL)
		return;

	if (sta->added_unassoc && (reply_res != WLAN_STATUS_SUCCESS ||
				   status != WLAN_STATUS_SUCCESS)) {
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->added_unassoc = 0;
		return;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return;

	hostapd_logger(hapd, dst, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG, "authentication OK (FT)");
	sta->flags |= WLAN_STA_AUTH;
	mlme_authenticate_indication(hapd, sta);
}
#endif /* CONFIG_IEEE80211R */


#ifdef CONFIG_SAE

#define dot11RSNASAESync 5		/* attempts */


static struct wpabuf * auth_build_sae_commit(struct hostapd_data *hapd,
					     struct sta_info *sta, int update)
{
	struct wpabuf *buf;

	if (hapd->conf->ssid.wpa_passphrase == NULL) {
		wpa_printf(MSG_DEBUG, "SAE: No password available");
		return NULL;
	}

	if (update &&
	    sae_prepare_commit(hapd->own_addr, sta->addr,
			       (u8 *) hapd->conf->ssid.wpa_passphrase,
			       os_strlen(hapd->conf->ssid.wpa_passphrase),
			       sta->sae) < 0) {
		wpa_printf(MSG_DEBUG, "SAE: Could not pick PWE");
		return NULL;
	}

	buf = wpabuf_alloc(SAE_COMMIT_MAX_LEN);
	if (buf == NULL)
		return NULL;
	sae_write_commit(sta->sae, buf, sta->sae->tmp ?
			 sta->sae->tmp->anti_clogging_token : NULL);

	return buf;
}


static struct wpabuf * auth_build_sae_confirm(struct hostapd_data *hapd,
					      struct sta_info *sta)
{
	struct wpabuf *buf;

	buf = wpabuf_alloc(SAE_CONFIRM_MAX_LEN);
	if (buf == NULL)
		return NULL;

	sae_write_confirm(sta->sae, buf);

	return buf;
}


static int auth_sae_send_commit(struct hostapd_data *hapd,
				struct sta_info *sta,
				const u8 *bssid, int update)
{
	struct wpabuf *data;
	int reply_res;

	data = auth_build_sae_commit(hapd, sta, update);
	if (data == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	reply_res = send_auth_reply(hapd, sta->addr, bssid, WLAN_AUTH_SAE, 1,
				    WLAN_STATUS_SUCCESS, wpabuf_head(data),
				    wpabuf_len(data));

	wpabuf_free(data);

	return reply_res;
}


static int auth_sae_send_confirm(struct hostapd_data *hapd,
				 struct sta_info *sta,
				 const u8 *bssid)
{
	struct wpabuf *data;
	int reply_res;

	data = auth_build_sae_confirm(hapd, sta);
	if (data == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	reply_res = send_auth_reply(hapd, sta->addr, bssid, WLAN_AUTH_SAE, 2,
				    WLAN_STATUS_SUCCESS, wpabuf_head(data),
				    wpabuf_len(data));

	wpabuf_free(data);

	return reply_res;
}


static int use_sae_anti_clogging(struct hostapd_data *hapd)
{
	struct sta_info *sta;
	unsigned int open = 0;

	if (hapd->conf->sae_anti_clogging_threshold == 0)
		return 1;

	for (sta = hapd->sta_list; sta; sta = sta->next) {
		if (!sta->sae)
			continue;
		if (sta->sae->state != SAE_COMMITTED &&
		    sta->sae->state != SAE_CONFIRMED)
			continue;
		open++;
		if (open >= hapd->conf->sae_anti_clogging_threshold)
			return 1;
	}

	return 0;
}


static int check_sae_token(struct hostapd_data *hapd, const u8 *addr,
			   const u8 *token, size_t token_len)
{
	u8 mac[SHA256_MAC_LEN];

	if (token_len != SHA256_MAC_LEN)
		return -1;
	if (hmac_sha256(hapd->sae_token_key, sizeof(hapd->sae_token_key),
			addr, ETH_ALEN, mac) < 0 ||
	    os_memcmp_const(token, mac, SHA256_MAC_LEN) != 0)
		return -1;

	return 0;
}


static struct wpabuf * auth_build_token_req(struct hostapd_data *hapd,
					    int group, const u8 *addr)
{
	struct wpabuf *buf;
	u8 *token;
	struct os_reltime now;

	os_get_reltime(&now);
	if (!os_reltime_initialized(&hapd->last_sae_token_key_update) ||
	    os_reltime_expired(&now, &hapd->last_sae_token_key_update, 60)) {
		if (random_get_bytes(hapd->sae_token_key,
				     sizeof(hapd->sae_token_key)) < 0)
			return NULL;
		wpa_hexdump(MSG_DEBUG, "SAE: Updated token key",
			    hapd->sae_token_key, sizeof(hapd->sae_token_key));
		hapd->last_sae_token_key_update = now;
	}

	buf = wpabuf_alloc(sizeof(le16) + SHA256_MAC_LEN);
	if (buf == NULL)
		return NULL;

	wpabuf_put_le16(buf, group); /* Finite Cyclic Group */

	token = wpabuf_put(buf, SHA256_MAC_LEN);
	hmac_sha256(hapd->sae_token_key, sizeof(hapd->sae_token_key),
		    addr, ETH_ALEN, token);

	return buf;
}


static int sae_check_big_sync(struct sta_info *sta)
{
	if (sta->sae->sync > dot11RSNASAESync) {
		sta->sae->state = SAE_NOTHING;
		sta->sae->sync = 0;
		return -1;
	}
	return 0;
}


static void auth_sae_retransmit_timer(void *eloop_ctx, void *eloop_data)
{
	struct hostapd_data *hapd = eloop_ctx;
	struct sta_info *sta = eloop_data;
	int ret;

	if (sae_check_big_sync(sta))
		return;
	sta->sae->sync++;
	wpa_printf(MSG_DEBUG, "SAE: Auth SAE retransmit timer for " MACSTR
		   " (sync=%d state=%d)",
		   MAC2STR(sta->addr), sta->sae->sync, sta->sae->state);

	switch (sta->sae->state) {
	case SAE_COMMITTED:
		ret = auth_sae_send_commit(hapd, sta, hapd->own_addr, 0);
		eloop_register_timeout(0,
				       hapd->dot11RSNASAERetransPeriod * 1000,
				       auth_sae_retransmit_timer, hapd, sta);
		break;
	case SAE_CONFIRMED:
		ret = auth_sae_send_confirm(hapd, sta, hapd->own_addr);
		eloop_register_timeout(0,
				       hapd->dot11RSNASAERetransPeriod * 1000,
				       auth_sae_retransmit_timer, hapd, sta);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret != WLAN_STATUS_SUCCESS)
		wpa_printf(MSG_INFO, "SAE: Failed to retransmit: ret=%d", ret);
}


void sae_clear_retransmit_timer(struct hostapd_data *hapd, struct sta_info *sta)
{
	eloop_cancel_timeout(auth_sae_retransmit_timer, hapd, sta);
}


static void sae_set_retransmit_timer(struct hostapd_data *hapd,
				     struct sta_info *sta)
{
	if (!(hapd->conf->mesh & MESH_ENABLED))
		return;

	eloop_cancel_timeout(auth_sae_retransmit_timer, hapd, sta);
	eloop_register_timeout(0, hapd->dot11RSNASAERetransPeriod * 1000,
			       auth_sae_retransmit_timer, hapd, sta);
}


void sae_accept_sta(struct hostapd_data *hapd, struct sta_info *sta)
{
	sta->flags |= WLAN_STA_AUTH;
	sta->auth_alg = WLAN_AUTH_SAE;
	mlme_authenticate_indication(hapd, sta);
	wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
	sta->sae->state = SAE_ACCEPTED;
	wpa_auth_pmksa_add_sae(hapd->wpa_auth, sta->addr,
			       sta->sae->pmk, sta->sae->pmkid);
}


static int sae_sm_step(struct hostapd_data *hapd, struct sta_info *sta,
		       const u8 *bssid, u8 auth_transaction)
{
	int ret;

	if (auth_transaction != 1 && auth_transaction != 2)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	switch (sta->sae->state) {
	case SAE_NOTHING:
		if (auth_transaction == 1) {
			ret = auth_sae_send_commit(hapd, sta, bssid, 1);
			if (ret)
				return ret;
			sta->sae->state = SAE_COMMITTED;

			if (sae_process_commit(sta->sae) < 0)
				return WLAN_STATUS_UNSPECIFIED_FAILURE;

			/*
			 * In mesh case, both Commit and Confirm can be sent
			 * immediately. In infrastructure BSS, only a single
			 * Authentication frame (Commit) is expected from the AP
			 * here and the second one (Confirm) will be sent once
			 * the STA has sent its second Authentication frame
			 * (Confirm).
			 */
			if (hapd->conf->mesh & MESH_ENABLED) {
				/*
				 * Send both Commit and Confirm immediately
				 * based on SAE finite state machine
				 * Nothing -> Confirm transition.
				 */
				ret = auth_sae_send_confirm(hapd, sta, bssid);
				if (ret)
					return ret;
				sta->sae->state = SAE_CONFIRMED;
			} else {
				/*
				 * For infrastructure BSS, send only the Commit
				 * message now to get alternating sequence of
				 * Authentication frames between the AP and STA.
				 * Confirm will be sent in
				 * Committed -> Confirmed/Accepted transition
				 * when receiving Confirm from STA.
				 */
			}
			sta->sae->sync = 0;
			sae_set_retransmit_timer(hapd, sta);
		} else {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_IEEE80211,
				       HOSTAPD_LEVEL_DEBUG,
				       "SAE confirm before commit");
		}
		break;
	case SAE_COMMITTED:
		sae_clear_retransmit_timer(hapd, sta);
		if (auth_transaction == 1) {
			if (sae_process_commit(sta->sae) < 0)
				return WLAN_STATUS_UNSPECIFIED_FAILURE;

			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;
			sta->sae->state = SAE_CONFIRMED;
			sta->sae->sync = 0;
			sae_set_retransmit_timer(hapd, sta);
		} else if (hapd->conf->mesh & MESH_ENABLED) {
			/*
			 * In mesh case, follow SAE finite state machine and
			 * send Commit now, if sync count allows.
			 */
			if (sae_check_big_sync(sta))
				return WLAN_STATUS_SUCCESS;
			sta->sae->sync++;

			ret = auth_sae_send_commit(hapd, sta, bssid, 0);
			if (ret)
				return ret;

			sae_set_retransmit_timer(hapd, sta);
		} else {
			/*
			 * For instructure BSS, send the postponed Confirm from
			 * Nothing -> Confirmed transition that was reduced to
			 * Nothing -> Committed above.
			 */
			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;

			sta->sae->state = SAE_CONFIRMED;

			/*
			 * Since this was triggered on Confirm RX, run another
			 * step to get to Accepted without waiting for
			 * additional events.
			 */
			return sae_sm_step(hapd, sta, bssid, auth_transaction);
		}
		break;
	case SAE_CONFIRMED:
		sae_clear_retransmit_timer(hapd, sta);
		if (auth_transaction == 1) {
			if (sae_check_big_sync(sta))
				return WLAN_STATUS_SUCCESS;
			sta->sae->sync++;

			ret = auth_sae_send_commit(hapd, sta, bssid, 1);
			if (ret)
				return ret;

			if (sae_process_commit(sta->sae) < 0)
				return WLAN_STATUS_UNSPECIFIED_FAILURE;

			ret = auth_sae_send_confirm(hapd, sta, bssid);
			if (ret)
				return ret;

			sae_set_retransmit_timer(hapd, sta);
		} else {
			sae_accept_sta(hapd, sta);
		}
		break;
	case SAE_ACCEPTED:
		if (auth_transaction == 1) {
			wpa_printf(MSG_DEBUG, "SAE: remove the STA (" MACSTR
				   ") doing reauthentication",
				   MAC2STR(sta->addr));
			ap_free_sta(hapd, sta);
			wpa_auth_pmksa_remove(hapd->wpa_auth, sta->addr);
		} else {
			if (sae_check_big_sync(sta))
				return WLAN_STATUS_SUCCESS;
			sta->sae->sync++;

			ret = auth_sae_send_confirm(hapd, sta, bssid);
			sae_clear_temp_data(sta->sae);
			if (ret)
				return ret;
		}
		break;
	default:
		wpa_printf_error(MSG_ERROR, "SAE: invalid state %d",
			   sta->sae->state);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
	return WLAN_STATUS_SUCCESS;
}


static void sae_pick_next_group(struct hostapd_data *hapd, struct sta_info *sta)
{
	struct sae_data *sae = sta->sae;
	int i, *groups = hapd->conf->sae_groups;

	if (sae->state != SAE_COMMITTED)
		return;

	wpa_printf(MSG_DEBUG, "SAE: Previously selected group: %d", sae->group);

	for (i = 0; groups && groups[i] > 0; i++) {
		if (sae->group == groups[i])
			break;
	}

	if (!groups || groups[i] <= 0) {
		wpa_printf(MSG_DEBUG,
			   "SAE: Previously selected group not found from the current configuration");
		return;
	}

	for (;;) {
		i++;
		if (groups[i] <= 0) {
			wpa_printf(MSG_DEBUG,
				   "SAE: No alternative group enabled");
			return;
		}

		if (sae_set_group(sae, groups[i]) < 0)
			continue;

		break;
	}
	wpa_printf(MSG_DEBUG, "SAE: Selected new group: %d", groups[i]);
}


static void handle_auth_sae(struct hostapd_data *hapd, struct sta_info *sta,
			    const struct ieee80211_mgmt *mgmt, size_t len,
			    u16 auth_transaction, u16 status_code)
{
	int resp = WLAN_STATUS_SUCCESS;
	struct wpabuf *data = NULL;

	if (!sta->sae) {
		if (auth_transaction != 1 ||
		    status_code != WLAN_STATUS_SUCCESS) {
			resp = -1;
			goto remove_sta;
		}
		sta->sae = os_zalloc(sizeof(*sta->sae));
		if (!sta->sae) {
			resp = -1;
			goto remove_sta;
		}
		sta->sae->state = SAE_NOTHING;
		sta->sae->sync = 0;
	}

	if (sta->mesh_sae_pmksa_caching) {
		wpa_printf(MSG_DEBUG,
			   "SAE: Cancel use of mesh PMKSA caching because peer starts SAE authentication");
		wpa_auth_pmksa_remove(hapd->wpa_auth, sta->addr);
		sta->mesh_sae_pmksa_caching = 0;
	}

	if (auth_transaction == 1) {
		const u8 *token = NULL, *pos, *end;
		size_t token_len = 0;
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "start SAE authentication (RX commit, status=%u)",
			       status_code);

		if ((hapd->conf->mesh & MESH_ENABLED) &&
		    status_code == WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ &&
		    sta->sae->tmp) {
			pos = mgmt->u.auth.variable;
			end = ((const u8 *) mgmt) + len;
			if (pos + sizeof(le16) > end) {
				wpa_printf_error(MSG_ERROR,
					   "SAE: Too short anti-clogging token request");
				resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto reply;
			}
			resp = sae_group_allowed(sta->sae,
						 hapd->conf->sae_groups,
						 WPA_GET_LE16(pos));
			if (resp != WLAN_STATUS_SUCCESS) {
				wpa_printf_error(MSG_ERROR,
					   "SAE: Invalid group in anti-clogging token request");
				goto reply;
			}
			pos += sizeof(le16);

			wpabuf_free(sta->sae->tmp->anti_clogging_token);
			sta->sae->tmp->anti_clogging_token =
				wpabuf_alloc_copy(pos, end - pos);
			if (sta->sae->tmp->anti_clogging_token == NULL) {
				wpa_printf_error(MSG_ERROR,
					   "SAE: Failed to alloc for anti-clogging token");
				resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto remove_sta;
			}

			/*
			 * IEEE Std 802.11-2012, 11.3.8.6.4: If the Status code
			 * is 76, a new Commit Message shall be constructed
			 * with the Anti-Clogging Token from the received
			 * Authentication frame, and the commit-scalar and
			 * COMMIT-ELEMENT previously sent.
			 */
			resp = auth_sae_send_commit(hapd, sta, mgmt->bssid, 0);
			if (resp != WLAN_STATUS_SUCCESS) {
				wpa_printf_error(MSG_ERROR,
					   "SAE: Failed to send commit message");
				goto remove_sta;
			}
			sta->sae->state = SAE_COMMITTED;
			sta->sae->sync = 0;
			sae_set_retransmit_timer(hapd, sta);
			return;
		}

		if ((hapd->conf->mesh & MESH_ENABLED) &&
		    status_code ==
		    WLAN_STATUS_FINITE_CYCLIC_GROUP_NOT_SUPPORTED &&
		    sta->sae->tmp) {
			wpa_printf(MSG_DEBUG,
				   "SAE: Peer did not accept our SAE group");
			sae_pick_next_group(hapd, sta);
			goto remove_sta;
		}

		if (status_code != WLAN_STATUS_SUCCESS)
			goto remove_sta;

		resp = sae_parse_commit(sta->sae, mgmt->u.auth.variable,
					((const u8 *) mgmt) + len -
					mgmt->u.auth.variable, &token,
					&token_len, hapd->conf->sae_groups);
		if (resp == SAE_SILENTLY_DISCARD) {
			wpa_printf(MSG_DEBUG,
				   "SAE: Drop commit message from " MACSTR " due to reflection attack",
				   MAC2STR(sta->addr));
			goto remove_sta;
		}
		if (token && check_sae_token(hapd, sta->addr, token, token_len)
		    < 0) {
			wpa_printf(MSG_DEBUG, "SAE: Drop commit message with "
				   "incorrect token from " MACSTR,
				   MAC2STR(sta->addr));
			resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
			goto remove_sta;
		}

		if (resp != WLAN_STATUS_SUCCESS)
			goto reply;

		if (!token && use_sae_anti_clogging(hapd)) {
			wpa_printf(MSG_DEBUG,
				   "SAE: Request anti-clogging token from "
				   MACSTR, MAC2STR(sta->addr));
			data = auth_build_token_req(hapd, sta->sae->group,
						    sta->addr);
			resp = WLAN_STATUS_ANTI_CLOGGING_TOKEN_REQ;
			if (hapd->conf->mesh & MESH_ENABLED)
				sta->sae->state = SAE_NOTHING;
			goto reply;
		}

		resp = sae_sm_step(hapd, sta, mgmt->bssid, auth_transaction);
	} else if (auth_transaction == 2) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "SAE authentication (RX confirm, status=%u)",
			       status_code);
		if (status_code != WLAN_STATUS_SUCCESS)
			goto remove_sta;
		if (sta->sae->state >= SAE_CONFIRMED ||
		    !(hapd->conf->mesh & MESH_ENABLED)) {
			if (sae_check_confirm(sta->sae, mgmt->u.auth.variable,
					      ((u8 *) mgmt) + len -
					      mgmt->u.auth.variable) < 0) {
				resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto reply;
			}
		}
		resp = sae_sm_step(hapd, sta, mgmt->bssid, auth_transaction);
	} else {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "unexpected SAE authentication transaction %u (status=%u)",
			       auth_transaction, status_code);
		if (status_code != WLAN_STATUS_SUCCESS)
			goto remove_sta;
		resp = WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION;
	}

reply:
	if (resp != WLAN_STATUS_SUCCESS) {
		send_auth_reply(hapd, mgmt->sa, mgmt->bssid, WLAN_AUTH_SAE,
				auth_transaction, resp,
				data ? wpabuf_head(data) : (u8 *) "",
				data ? wpabuf_len(data) : 0);
	}

remove_sta:
	if (sta->added_unassoc && (resp != WLAN_STATUS_SUCCESS ||
				   status_code != WLAN_STATUS_SUCCESS)) {
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->added_unassoc = 0;
	}
	wpabuf_free(data);
}


/**
 * auth_sae_init_committed - Send COMMIT and start SAE in committed state
 * @hapd: BSS data for the device initiating the authentication
 * @sta: the peer to which commit authentication frame is sent
 *
 * This function implements Init event handling (IEEE Std 802.11-2012,
 * 11.3.8.6.3) in which initial COMMIT message is sent. Prior to calling, the
 * sta->sae structure should be initialized appropriately via a call to
 * sae_prepare_commit().
 */
int auth_sae_init_committed(struct hostapd_data *hapd, struct sta_info *sta)
{
	int ret;

	if (!sta->sae || !sta->sae->tmp)
		return -1;

	if (sta->sae->state != SAE_NOTHING)
		return -1;

	ret = auth_sae_send_commit(hapd, sta, hapd->own_addr, 0);
	if (ret)
		return -1;

	sta->sae->state = SAE_COMMITTED;
	sta->sae->sync = 0;
	sae_set_retransmit_timer(hapd, sta);

	return 0;
}

#endif /* CONFIG_SAE */


static void handle_auth(struct hostapd_data *hapd,
			const struct ieee80211_mgmt *mgmt, size_t len)
{
	u16 auth_alg, auth_transaction, status_code;
	u16 resp = WLAN_STATUS_SUCCESS;
	struct sta_info *sta = NULL;
	int reply_res;
	u16 fc;
	const u8 *challenge = NULL;
	//u32 session_timeout, acct_interim_interval;
	//struct hostapd_sta_wpa_psk_short *psk = NULL;
	u8 resp_ies[2 + WLAN_AUTH_CHALLENGE_LEN];
	size_t resp_ies_len = 0;
	//char *identity = NULL;
	//char *radius_cui = NULL;
	u16 seq_ctrl;


	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.auth)) {
		wpa_printf(MSG_INFO, "handle_auth - too short payload (len=%lu)",
			   (unsigned long) len);
		return;
	}

#ifdef CONFIG_TESTING_OPTIONS
	if (hapd->iconf->ignore_auth_probability > 0.0 &&
	    drand48() < hapd->iconf->ignore_auth_probability) {
		wpa_printf(MSG_INFO,
			   "TESTING: ignoring auth frame from " MACSTR,
			   MAC2STR(mgmt->sa));
		return;
	}
#endif /* CONFIG_TESTING_OPTIONS */

	auth_alg = le_to_host16(mgmt->u.auth.auth_alg);
	auth_transaction = le_to_host16(mgmt->u.auth.auth_transaction);
	status_code = le_to_host16(mgmt->u.auth.status_code);
	fc = le_to_host16(mgmt->frame_control);
	seq_ctrl = le_to_host16(mgmt->seq_ctrl);

	if (len >= IEEE80211_HDRLEN + sizeof(mgmt->u.auth) +
	    2 + WLAN_AUTH_CHALLENGE_LEN &&
	    mgmt->u.auth.variable[0] == WLAN_EID_CHALLENGE &&
	    mgmt->u.auth.variable[1] == WLAN_AUTH_CHALLENGE_LEN)
		challenge = &mgmt->u.auth.variable[2];

	wpa_printf(MSG_DEBUG, "authentication: STA=" MACSTR " auth_alg=%d "
		   "auth_transaction=%d status_code=%d wep=%d%s "
		   "seq_ctrl=0x%x%s",
		   MAC2STR(mgmt->sa), auth_alg, auth_transaction,
		   status_code, !!(fc & WLAN_FC_ISWEP),
		   challenge ? " challenge" : "",
		   seq_ctrl, (fc & WLAN_FC_RETRY) ? " retry" : "");

#ifdef CONFIG_NO_RC4
	if (auth_alg == WLAN_AUTH_SHARED_KEY) {
		wpa_printf(MSG_INFO,
			   "Unsupported authentication algorithm (%d)",
			   auth_alg);
		resp = WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
		goto fail;
	}
#endif /* CONFIG_NO_RC4 */

	if (hapd->tkip_countermeasures) {
	    wpa_printf(MSG_DEBUG,
			   "Ongoing TKIP countermeasures (Michael MIC failure) - reject authentication");
		resp = WLAN_REASON_MICHAEL_MIC_FAILURE;
		goto fail;
	}

	if (!(((hapd->conf->auth_algs & WPA_AUTH_ALG_OPEN) &&
	       auth_alg == WLAN_AUTH_OPEN) ||
#ifdef CONFIG_IEEE80211R
	      (hapd->conf->wpa && wpa_key_mgmt_ft(hapd->conf->wpa_key_mgmt) &&
	       auth_alg == WLAN_AUTH_FT) ||
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_SAE
	      (hapd->conf->wpa && wpa_key_mgmt_sae(hapd->conf->wpa_key_mgmt) &&
	       auth_alg == WLAN_AUTH_SAE) ||
#endif /* CONFIG_SAE */
	      ((hapd->conf->auth_algs & WPA_AUTH_ALG_SHARED) &&
	       auth_alg == WLAN_AUTH_SHARED_KEY))) {
		wpa_printf(MSG_INFO, "Unsupported authentication algorithm (%d)",
			   auth_alg);
		resp = WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
		goto fail;
	}

	if (!(auth_transaction == 1 || auth_alg == WLAN_AUTH_SAE ||
	      (auth_alg == WLAN_AUTH_SHARED_KEY && auth_transaction == 3))) {
		wpa_printf(MSG_INFO, "Unknown authentication transaction number (%d)",
			   auth_transaction);
		resp = WLAN_STATUS_UNKNOWN_AUTH_TRANSACTION;
		goto fail;
	}

	if (os_memcmp(mgmt->sa, hapd->own_addr, ETH_ALEN) == 0) {
		wpa_printf(MSG_INFO, "Station " MACSTR " not allowed to authenticate",
			   MAC2STR(mgmt->sa));
		resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
		goto fail;
	}

#if 0
	if (hapd->conf->no_auth_if_seen_on) {
		struct hostapd_data *other;

		other = sta_track_seen_on(hapd->iface, mgmt->sa,
					  hapd->conf->no_auth_if_seen_on);
		if (other) {
			u8 *pos;
			u32 info;
			u8 op_class, channel, phytype;

			wpa_printf(MSG_DEBUG, "%s: Reject authentication from "
				   MACSTR " since STA has been seen on %s",
				   hapd->conf->iface, MAC2STR(mgmt->sa),
				   hapd->conf->no_auth_if_seen_on);

			resp = WLAN_STATUS_REJECTED_WITH_SUGGESTED_BSS_TRANSITION;
			pos = &resp_ies[0];
			*pos++ = WLAN_EID_NEIGHBOR_REPORT;
			*pos++ = 13;
			os_memcpy(pos, other->own_addr, ETH_ALEN);
			pos += ETH_ALEN;
			info = 0; /* TODO: BSSID Information */
			WPA_PUT_LE32(pos, info);
			pos += 4;
			if (other->iconf->hw_mode == HOSTAPD_MODE_IEEE80211AD)
				phytype = 8; /* dmg */
			else if (other->iconf->ieee80211ac)
				phytype = 9; /* vht */
			else if (other->iconf->ieee80211n)
				phytype = 7; /* ht */
			else if (other->iconf->hw_mode ==
				 HOSTAPD_MODE_IEEE80211A)
				phytype = 4; /* ofdm */
			else if (other->iconf->hw_mode ==
				 HOSTAPD_MODE_IEEE80211G)
				phytype = 6; /* erp */
			else
				phytype = 5; /* hrdsss */
			if (ieee80211_freq_to_channel_ext(
				    hostapd_hw_get_freq(other,
							other->iconf->channel),
				    other->iconf->secondary_channel,
				    other->iconf->ieee80211ac,
				    &op_class, &channel) == NUM_HOSTAPD_MODES) {
				op_class = 0;
				channel = other->iconf->channel;
			}
			*pos++ = op_class;
			*pos++ = channel;
			*pos++ = phytype;
			resp_ies_len = pos - &resp_ies[0];
			goto fail;
		}
	}
#endif
	sta = ap_get_sta(hapd, mgmt->sa);
	if (sta) {
		if ((fc & WLAN_FC_RETRY) &&
		    sta->last_seq_ctrl != WLAN_INVALID_MGMT_SEQ &&
		    sta->last_seq_ctrl == seq_ctrl &&
		    sta->last_subtype == WLAN_FC_STYPE_AUTH) {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_IEEE80211,
				       HOSTAPD_LEVEL_DEBUG,
				       "Drop repeated authentication frame seq_ctrl=0x%x",
				       seq_ctrl);
			return;
		}
#ifdef CONFIG_MESH
		if ((hapd->conf->mesh & MESH_ENABLED) &&
		    sta->plink_state == PLINK_BLOCKED) {
			wpa_printf(MSG_DEBUG, "Mesh peer " MACSTR
				   " is blocked - drop Authentication frame",
				   MAC2STR(mgmt->sa));
			return;
		}
#endif /* CONFIG_MESH */
	} else {
#ifdef CONFIG_MESH
		if (hapd->conf->mesh & MESH_ENABLED) {
			/* if the mesh peer is not available, we don't do auth.
			 */
			wpa_printf(MSG_DEBUG, "Mesh peer " MACSTR
				   " not yet known - drop Authentication frame",
				   MAC2STR(mgmt->sa));
			/*
			 * Save a copy of the frame so that it can be processed
			 * if a new peer entry is added shortly after this.
			 */
			wpabuf_free(hapd->mesh_pending_auth);
			hapd->mesh_pending_auth = wpabuf_alloc_copy(mgmt, len);
			os_get_reltime(&hapd->mesh_pending_auth_time);
			return;
		}
#endif /* CONFIG_MESH */

		sta = ap_sta_add(hapd, mgmt->sa);
		if (!sta) {
			resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
			goto fail;
		}
	}
	sta->last_seq_ctrl = seq_ctrl;
	sta->last_subtype = WLAN_FC_STYPE_AUTH;

	//identity = NULL;
	//radius_cui = NULL;

	sta->flags &= ~WLAN_STA_PREAUTH;
	//ieee802_1x_notify_pre_auth(sta->eapol_sm, 0);

    ap_sta_no_session_timeout(hapd, sta);

	/*
	 * If the driver supports full AP client state, add a station to the
	 * driver before sending authentication reply to make sure the driver
	 * has resources, and not to go through the entire authentication and
	 * association handshake, and fail it at the end.
	 *
	 * If this is not the first transaction, in a multi-step authentication
	 * algorithm, the station already exists in the driver
	 * (sta->added_unassoc = 1) so skip it.
	 *
	 * In mesh mode, the station was already added to the driver when the
	 * NEW_PEER_CANDIDATE event is received.
	 */
#if 0	 
	if (FULL_AP_CLIENT_STATE_SUPP(hapd->iface->drv_flags) &&
	    !(sta->added_unassoc)) {
		/*
		 * If a station that is already associated to the AP, is trying
		 * to authenticate again, remove the STA entry, in order to make
		 * sure the STA PS state gets cleared and configuration gets
		 * updated. To handle this, station's added_unassoc flag is
		 * cleared once the station has completed association.
		 */
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->flags &= ~(WLAN_STA_ASSOC | WLAN_STA_AUTH |
				WLAN_STA_AUTHORIZED);

		if (hostapd_sta_add(hapd, sta->addr, 0, 0, NULL, 0, 0,
				    NULL, NULL, sta->flags, 0, 0, 0, 0)) {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_IEEE80211,
				       HOSTAPD_LEVEL_NOTICE,
				       "Could not add STA to kernel driver");
			resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
			goto fail;
		}

		sta->added_unassoc = 1;
	}
#endif
	switch (auth_alg) {
	case WLAN_AUTH_OPEN:
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "authentication OK (open system)");
		sta->flags |= WLAN_STA_AUTH;
		wpa_auth_sm_event(sta->wpa_sm, WPA_AUTH);
		sta->auth_alg = WLAN_AUTH_OPEN;
		mlme_authenticate_indication(hapd, sta);
		break;
#ifndef CONFIG_NO_RC4
	case WLAN_AUTH_SHARED_KEY:
		resp = auth_shared_key(hapd, sta, auth_transaction, challenge,
				       fc & WLAN_FC_ISWEP);
		sta->auth_alg = WLAN_AUTH_SHARED_KEY;
		mlme_authenticate_indication(hapd, sta);
		if (sta->challenge && auth_transaction == 1) {
			resp_ies[0] = WLAN_EID_CHALLENGE;
			resp_ies[1] = WLAN_AUTH_CHALLENGE_LEN;
			os_memcpy(resp_ies + 2, sta->challenge,
				  WLAN_AUTH_CHALLENGE_LEN);
			resp_ies_len = 2 + WLAN_AUTH_CHALLENGE_LEN;
		}
		break;
#endif /* CONFIG_NO_RC4 */
#ifdef CONFIG_IEEE80211R
	case WLAN_AUTH_FT:
		sta->auth_alg = WLAN_AUTH_FT;
		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr, NULL);
		if (sta->wpa_sm == NULL) {
			wpa_printf(MSG_DEBUG, "FT: Failed to initialize WPA "
				   "state machine");
			resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
			goto fail;
		}
		wpa_ft_process_auth(sta->wpa_sm, mgmt->bssid,
				    auth_transaction, mgmt->u.auth.variable,
				    len - IEEE80211_HDRLEN -
				    sizeof(mgmt->u.auth),
				    handle_auth_ft_finish, hapd);
		/* handle_auth_ft_finish() callback will complete auth. */
		return;
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_SAE
	case WLAN_AUTH_SAE:
#ifdef CONFIG_MESH
		if (status_code == WLAN_STATUS_SUCCESS &&
		    hapd->conf->mesh & MESH_ENABLED) {
			if (sta->wpa_sm == NULL)
				sta->wpa_sm =
					wpa_auth_sta_init(hapd->wpa_auth,
							  sta->addr, NULL);
			if (sta->wpa_sm == NULL) {
				wpa_printf(MSG_DEBUG,
					   "SAE: Failed to initialize WPA state machine");
				resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
				goto fail;
			}
		}
#endif /* CONFIG_MESH */
		handle_auth_sae(hapd, sta, mgmt, len, auth_transaction,
				status_code);
		return;
#endif /* CONFIG_SAE */
	}

 fail:
	//os_free(identity);
	//os_free(radius_cui);
	//hostapd_free_psk_list(psk);

	reply_res = send_auth_reply(hapd, mgmt->sa, mgmt->bssid, auth_alg,
				    auth_transaction + 1, resp, resp_ies,
				    resp_ies_len);

	if (sta && sta->added_unassoc && (resp != WLAN_STATUS_SUCCESS ||
					  reply_res != WLAN_STATUS_SUCCESS)) {
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->added_unassoc = 0;
	}
}


int hostapd_get_aid(struct hostapd_data *hapd, struct sta_info *sta)
{
	int i, j = 32, aid;

	/* get a unique AID */
	if (sta->aid > 0) {
		wpa_printf(MSG_DEBUG, "  old AID %d", sta->aid);
		return 0;
	}

	if (TEST_FAIL())
		return -1;

	for (i = 0; i < AID_WORDS; i++) {
		if (hapd->sta_aid[i] == (u32) -1)
			continue;
		for (j = 0; j < 32; j++) {
			if (!(hapd->sta_aid[i] & BIT(j)))
				break;
		}
		if (j < 32)
			break;
	}
	if (j == 32)
		return -1;
	aid = i * 32 + j + 1;
#if 0    
	if (aid > 2007)
		return -1;
#else  // for wpa reduce.
    if (aid > 32)
        return -1;
#endif

	sta->aid = aid;
	hapd->sta_aid[i] |= BIT(j);
	wpa_printf(MSG_DEBUG, "  new AID %d", sta->aid);
	return 0;
}


static u16 check_ssid(struct hostapd_data *hapd, struct sta_info *sta,
		      const u8 *ssid_ie, size_t ssid_ie_len)
{
	if (ssid_ie == NULL)
		return WLAN_STATUS_UNSPECIFIED_FAILURE;

	if (ssid_ie_len != hapd->conf->ssid.ssid_len ||
	    os_memcmp(ssid_ie, hapd->conf->ssid.ssid, ssid_ie_len) != 0) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO,
			       "Station tried to associate with unknown SSID "
			       "'%s'", wpa_ssid_txt(ssid_ie, ssid_ie_len));
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}


static u16 check_wmm(struct hostapd_data *hapd, struct sta_info *sta,
		     const u8 *wmm_ie, size_t wmm_ie_len)
{
#if 1
	sta->flags &= ~WLAN_STA_WMM;
	sta->qosinfo = 0;
	if (wmm_ie && hapd->conf->wmm_enabled) {
		struct wmm_information_element *wmm;

		if (!hostapd_eid_wmm_valid(hapd, wmm_ie, wmm_ie_len)) {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_WPA,
				       HOSTAPD_LEVEL_DEBUG,
				       "invalid WMM element in association "
				       "request");
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
		}

		sta->flags |= WLAN_STA_WMM;
		wmm = (struct wmm_information_element *) wmm_ie;
		sta->qosinfo = wmm->qos_info;
	}
#endif    
	return WLAN_STATUS_SUCCESS;
}

#if 1
static u16 copy_supp_rates(struct hostapd_data *hapd, struct sta_info *sta,
			   struct ieee802_11_elems *elems)
{
	if (!elems->supp_rates) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "No supported rates element in AssocReq");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	if (elems->supp_rates_len + elems->ext_supp_rates_len >
	    sizeof(sta->supported_rates)) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "Invalid supported rates element length %d+%d",
			       elems->supp_rates_len,
			       elems->ext_supp_rates_len);
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	sta->supported_rates_len = merge_byte_arrays(
		sta->supported_rates, sizeof(sta->supported_rates),
		elems->supp_rates, elems->supp_rates_len,
		elems->ext_supp_rates, elems->ext_supp_rates_len);

	return WLAN_STATUS_SUCCESS;
}
#endif

static u16 check_ext_capab(struct hostapd_data *hapd, struct sta_info *sta,
			   const u8 *ext_capab_ie, size_t ext_capab_ie_len)
{
#ifdef CONFIG_INTERWORKING
	/* check for QoS Map support */
	if (ext_capab_ie_len >= 5) {
		if (ext_capab_ie[4] & 0x01)
			sta->qos_map_enabled = 1;
	}
#endif /* CONFIG_INTERWORKING */

	if (ext_capab_ie_len > 0)
		sta->ecsa_supported = !!(ext_capab_ie[0] & BIT(2));

	return WLAN_STATUS_SUCCESS;
}


static u16 check_assoc_ies(struct hostapd_data *hapd, struct sta_info *sta,
			   const u8 *ies, size_t ies_len, int reassoc)
{
	struct ieee802_11_elems elems;
	u16 resp;
	const u8 *wpa_ie;
	size_t wpa_ie_len;
	const u8 *p2p_dev_addr = NULL;

	if (ieee802_11_parse_elems(ies, ies_len, &elems, 1) == ParseFailed) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "Station sent an invalid "
			       "association request");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	resp = check_ssid(hapd, sta, elems.ssid, elems.ssid_len);
	if (resp != WLAN_STATUS_SUCCESS)
		return resp;
	resp = check_wmm(hapd, sta, elems.wmm, elems.wmm_len);
	if (resp != WLAN_STATUS_SUCCESS)
		return resp;
	resp = check_ext_capab(hapd, sta, elems.ext_capab, elems.ext_capab_len);
	if (resp != WLAN_STATUS_SUCCESS)
		return resp;
	resp = copy_supp_rates(hapd, sta, &elems);
	if (resp != WLAN_STATUS_SUCCESS)
		return resp;
#ifdef CONFIG_IEEE80211N
	resp = copy_sta_ht_capab(hapd, sta, elems.ht_capabilities);
	if (resp != WLAN_STATUS_SUCCESS)
		return resp;
    wpa_printf(MSG_DEBUG, "%s STA " MACSTR " - %s",
			   __func__, MAC2STR(sta->addr),
			   sta->flags & WLAN_STA_HT ? "has ht" : "no ht");
	/*if (hapd->iconf->ieee80211n && hapd->iconf->require_ht &&
	    !(sta->flags & WLAN_STA_HT)) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "Station does not support "
			       "mandatory HT PHY - reject association");
		return WLAN_STATUS_ASSOC_DENIED_NO_HT;
	}*/
#endif /* CONFIG_IEEE80211N */

#ifdef CONFIG_IEEE80211AC
	if (hapd->iconf->ieee80211ac) {
		resp = copy_sta_vht_capab(hapd, sta, elems.vht_capabilities);
		if (resp != WLAN_STATUS_SUCCESS)
			return resp;

		resp = set_sta_vht_opmode(hapd, sta, elems.vht_opmode_notif);
		if (resp != WLAN_STATUS_SUCCESS)
			return resp;
	}

	if (hapd->iconf->ieee80211ac && hapd->iconf->require_vht &&
	    !(sta->flags & WLAN_STA_VHT)) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "Station does not support "
			       "mandatory VHT PHY - reject association");
		return WLAN_STATUS_ASSOC_DENIED_NO_VHT;
	}

	if (hapd->conf->vendor_vht && !elems.vht_capabilities) {
		resp = copy_sta_vendor_vht(hapd, sta, elems.vendor_vht,
					   elems.vendor_vht_len);
		if (resp != WLAN_STATUS_SUCCESS)
			return resp;
	}
#endif /* CONFIG_IEEE80211AC */

#ifdef CONFIG_IEEE80211AX
	if (hapd->iconf->ieee80211ax) {
		resp = copy_sta_he_capab(hapd, sta, IEEE80211_MODE_AP,
					 elems.he_capabilities,
					 elems.he_capabilities_len);
		if (resp != WLAN_STATUS_SUCCESS)
			return resp;
        wpa_printf(MSG_DEBUG, "%s STA " MACSTR " - %s",
			   __func__, MAC2STR(sta->addr),
			   sta->flags & WLAN_STA_HE ? "has he" : "no he");
	}
#endif /* CONFIG_IEEE80211AX */

#ifdef CONFIG_P2P
	if (elems.p2p) {
		wpabuf_free(sta->p2p_ie);
		sta->p2p_ie = ieee802_11_vendor_ie_concat(ies, ies_len,
							  P2P_IE_VENDOR_TYPE);
		if (sta->p2p_ie)
			p2p_dev_addr = p2p_get_go_dev_addr(sta->p2p_ie);
	} else {
		wpabuf_free(sta->p2p_ie);
		sta->p2p_ie = NULL;
	}
#endif /* CONFIG_P2P */

	if ((hapd->conf->wpa & WPA_PROTO_RSN) && elems.rsn_ie) {
		wpa_ie = elems.rsn_ie;
		wpa_ie_len = elems.rsn_ie_len;
	} else if ((hapd->conf->wpa & WPA_PROTO_WPA) &&
		   elems.wpa_ie) {
		wpa_ie = elems.wpa_ie;
		wpa_ie_len = elems.wpa_ie_len;
	} else {
		wpa_ie = NULL;
		wpa_ie_len = 0;
	}

#ifdef CONFIG_WPS
	sta->flags &= ~(WLAN_STA_WPS | WLAN_STA_MAYBE_WPS | WLAN_STA_WPS2);
	if (hapd->conf->wps_state && elems.wps_ie) {
		wpa_printf(MSG_DEBUG, "STA included WPS IE in (Re)Association "
			   "Request - assume WPS is used");
		sta->flags |= WLAN_STA_WPS;
		wpabuf_free(sta->wps_ie);
		sta->wps_ie = ieee802_11_vendor_ie_concat(ies, ies_len,
							  WPS_IE_VENDOR_TYPE);
		if (sta->wps_ie && wps_is_20(sta->wps_ie)) {
			wpa_printf(MSG_DEBUG, "WPS: STA supports WPS 2.0");
			sta->flags |= WLAN_STA_WPS2;
		}
		wpa_ie = NULL;
		wpa_ie_len = 0;
		if (sta->wps_ie && wps_validate_assoc_req(sta->wps_ie) < 0) {
			wpa_printf(MSG_DEBUG, "WPS: Invalid WPS IE in "
				   "(Re)Association Request - reject");
			return WLAN_STATUS_INVALID_IE;
		}
	} else if (hapd->conf->wps_state && wpa_ie == NULL) {
		wpa_printf(MSG_DEBUG, "STA did not include WPA/RSN IE in "
			   "(Re)Association Request - possible WPS use");
		sta->flags |= WLAN_STA_MAYBE_WPS;
	} else
#endif /* CONFIG_WPS */
	if (hapd->conf->wpa && wpa_ie == NULL) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO,
			       "No WPA/RSN IE in association request");
		return WLAN_STATUS_INVALID_IE;
	}

	if (hapd->conf->wpa && wpa_ie) {
		int res;
		wpa_ie -= 2;
		wpa_ie_len += 2;
		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr,
							p2p_dev_addr);
		if (sta->wpa_sm == NULL) {
			wpa_printf_warning(MSG_WARNING, "Failed to initialize WPA "
				   "state machine");
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
		}
		res = wpa_validate_wpa_ie(hapd->wpa_auth, sta->wpa_sm,
					  wpa_ie, wpa_ie_len,
					  elems.mdie, elems.mdie_len);
		if (res == WPA_INVALID_GROUP)
			resp = WLAN_STATUS_GROUP_CIPHER_NOT_VALID;
		else if (res == WPA_INVALID_PAIRWISE)
			resp = WLAN_STATUS_PAIRWISE_CIPHER_NOT_VALID;
		else if (res == WPA_INVALID_AKMP)
			resp = WLAN_STATUS_AKMP_NOT_VALID;
		else if (res == WPA_ALLOC_FAIL)
			resp = WLAN_STATUS_UNSPECIFIED_FAILURE;
#ifdef CONFIG_IEEE80211W
		else if (res == WPA_MGMT_FRAME_PROTECTION_VIOLATION)
			resp = WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION;
		else if (res == WPA_INVALID_MGMT_GROUP_CIPHER)
			resp = WLAN_STATUS_ROBUST_MGMT_FRAME_POLICY_VIOLATION;
#endif /* CONFIG_IEEE80211W */
		else if (res == WPA_INVALID_MDIE)
			resp = WLAN_STATUS_INVALID_MDIE;
		else if (res != WPA_IE_OK)
			resp = WLAN_STATUS_INVALID_IE;
		if (resp != WLAN_STATUS_SUCCESS)
			return resp;
#ifdef CONFIG_IEEE80211W
		if ((sta->flags & WLAN_STA_MFP) && !sta->sa_query_timed_out &&
		    sta->sa_query_count > 0)
			ap_check_sa_query_timeout(hapd, sta);
		if ((sta->flags & WLAN_STA_MFP) && !sta->sa_query_timed_out &&
		    (!reassoc || sta->auth_alg != WLAN_AUTH_FT)) {
			/*
			 * STA has already been associated with MFP and SA
			 * Query timeout has not been reached. Reject the
			 * association attempt temporarily and start SA Query,
			 * if one is not pending.
			 */

			if (sta->sa_query_count == 0)
				ap_sta_start_sa_query(hapd, sta);

			return WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY;
		}

		if (wpa_auth_uses_mfp(sta->wpa_sm))
			sta->flags |= WLAN_STA_MFP;
		else
			sta->flags &= ~WLAN_STA_MFP;
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_IEEE80211R
		if (sta->auth_alg == WLAN_AUTH_FT) {
			if (!reassoc) {
				wpa_printf(MSG_DEBUG, "FT: " MACSTR " tried "
					   "to use association (not "
					   "re-association) with FT auth_alg",
					   MAC2STR(sta->addr));
				return WLAN_STATUS_UNSPECIFIED_FAILURE;
			}

			resp = wpa_ft_validate_reassoc(sta->wpa_sm, ies,
						       ies_len);
			if (resp != WLAN_STATUS_SUCCESS)
				return resp;
		}
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_SAE
		if (wpa_auth_uses_sae(sta->wpa_sm) &&
		    sta->auth_alg == WLAN_AUTH_OPEN) {
			struct rsn_pmksa_cache_entry *sa;
			sa = wpa_auth_sta_get_pmksa(sta->wpa_sm);
			if (!sa || sa->akmp != WPA_KEY_MGMT_SAE) {
				wpa_printf(MSG_DEBUG,
					   "SAE: No PMKSA cache entry found for "
					   MACSTR, MAC2STR(sta->addr));
				return WLAN_STATUS_INVALID_PMKID;
			}
			wpa_printf(MSG_DEBUG, "SAE: " MACSTR
				   " using PMKSA caching", MAC2STR(sta->addr));
		} else if (wpa_auth_uses_sae(sta->wpa_sm) &&
			   sta->auth_alg != WLAN_AUTH_SAE &&
			   !(sta->auth_alg == WLAN_AUTH_FT &&
			     wpa_auth_uses_ft_sae(sta->wpa_sm))) {
			wpa_printf(MSG_DEBUG, "SAE: " MACSTR " tried to use "
				   "SAE AKM after non-SAE auth_alg %u",
				   MAC2STR(sta->addr), sta->auth_alg);
			return WLAN_STATUS_NOT_SUPPORTED_AUTH_ALG;
		}
#endif /* CONFIG_SAE */

#ifdef CONFIG_IEEE80211N
		if ((sta->flags & (WLAN_STA_HT | WLAN_STA_VHT)) &&
		    wpa_auth_get_pairwise(sta->wpa_sm) == WPA_CIPHER_TKIP) {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_IEEE80211,
				       HOSTAPD_LEVEL_INFO,
				       "Station tried to use TKIP with HT "
				       "association");
			return WLAN_STATUS_CIPHER_REJECTED_PER_POLICY;
		}
#endif /* CONFIG_IEEE80211N */
#ifdef CONFIG_HS20
	} else if (hapd->conf->osen) {
		if (elems.osen == NULL) {
			hostapd_logger(
				hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
				HOSTAPD_LEVEL_INFO,
				"No HS 2.0 OSEN element in association request");
			return WLAN_STATUS_INVALID_IE;
		}

		wpa_printf(MSG_DEBUG, "HS 2.0: OSEN association");
		if (sta->wpa_sm == NULL)
			sta->wpa_sm = wpa_auth_sta_init(hapd->wpa_auth,
							sta->addr, NULL);
		if (sta->wpa_sm == NULL) {
			wpa_printf_warning(MSG_WARNING, "Failed to initialize WPA "
				   "state machine");
			return WLAN_STATUS_UNSPECIFIED_FAILURE;
		}
		if (wpa_validate_osen(hapd->wpa_auth, sta->wpa_sm,
				      elems.osen - 2, elems.osen_len + 2) < 0)
			return WLAN_STATUS_INVALID_IE;
#endif /* CONFIG_HS20 */
	} else
		wpa_auth_sta_no_wpa(sta->wpa_sm);

#ifdef CONFIG_P2P
	p2p_group_notif_assoc(hapd->p2p_group, sta->addr, ies, ies_len);
#endif /* CONFIG_P2P */

#ifdef CONFIG_HS20
	wpabuf_free(sta->hs20_ie);
	if (elems.hs20 && elems.hs20_len > 4) {
		sta->hs20_ie = wpabuf_alloc_copy(elems.hs20 + 4,
						 elems.hs20_len - 4);
	} else
		sta->hs20_ie = NULL;
#endif /* CONFIG_HS20 */

#ifdef CONFIG_FST
	wpabuf_free(sta->mb_ies);
	if (hapd->iface->fst)
		sta->mb_ies = mb_ies_by_info(&elems.mb_ies);
	else
		sta->mb_ies = NULL;
#endif /* CONFIG_FST */

#ifdef CONFIG_MBO
	mbo_ap_check_sta_assoc(hapd, sta, &elems);

	if (hapd->conf->mbo_enabled && (hapd->conf->wpa & 2) &&
	    elems.mbo && sta->cell_capa && !(sta->flags & WLAN_STA_MFP) &&
	    hapd->conf->ieee80211w != NO_MGMT_FRAME_PROTECTION) {
		wpa_printf(MSG_INFO,
			   "MBO: Reject WPA2 association without PMF");
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}
#endif /* CONFIG_MBO */

	//ap_copy_sta_supp_op_classes(sta, elems.supp_op_classes,
	//			    elems.supp_op_classes_len);

	return WLAN_STATUS_SUCCESS;
}


static void send_deauth(struct hostapd_data *hapd, const u8 *addr,
			u16 reason_code)
{
	int send_len;
	struct ieee80211_mgmt reply;

	os_memset(&reply, 0, sizeof(reply));
	reply.frame_control =
		IEEE80211_FC(WLAN_FC_TYPE_MGMT, WLAN_FC_STYPE_DEAUTH);
	os_memcpy(reply.da, addr, ETH_ALEN);
	os_memcpy(reply.sa, hapd->own_addr, ETH_ALEN);
	os_memcpy(reply.bssid, hapd->own_addr, ETH_ALEN);

	send_len = IEEE80211_HDRLEN + sizeof(reply.u.deauth);
	reply.u.deauth.reason_code = host_to_le16(reason_code);

	if (hostapd_drv_send_mlme(hapd, &reply, send_len, 0) < 0)
		wpa_printf(MSG_INFO, "Failed to send deauth: %s",
			   strerror(errno));
}

extern void hostapd_get_he_capab(struct hostapd_data *hapd,
						  const struct ieee80211_he_capabilities *he_cap,
						  struct ieee80211_he_capabilities *neg_he_cap,
						  size_t he_capab_len);
static int add_associated_sta(struct hostapd_data *hapd,
			      struct sta_info *sta)
{
	struct ieee80211_ht_capabilities ht_cap;
	struct ieee80211_vht_capabilities vht_cap;
	struct ieee80211_he_capabilities he_cap;

	/*
	 * Remove the STA entry to ensure the STA PS state gets cleared and
	 * configuration gets updated. This is relevant for cases, such as
	 * FT-over-the-DS, where a station re-associates back to the same AP but
	 * skips the authentication flow, or if working with a driver that
	 * does not support full AP client state.
	 */
	if (!sta->added_unassoc)
		hostapd_drv_sta_remove(hapd, sta->addr);

#ifdef CONFIG_IEEE80211N
	if (sta->flags & WLAN_STA_HT)
		hostapd_get_ht_capab(hapd, sta->ht_capabilities, &ht_cap);
#endif /* CONFIG_IEEE80211N */
#ifdef CONFIG_IEEE80211AC
	if (sta->flags & WLAN_STA_VHT)
		hostapd_get_vht_capab(hapd, sta->vht_capabilities, &vht_cap);
#endif /* CONFIG_IEEE80211AC */
#ifdef CONFIG_IEEE80211AX
	if (sta->flags & WLAN_STA_HE) {
		hostapd_get_he_capab(hapd, sta->he_capab, &he_cap,
				     sta->he_capab_len);
	}
#endif /* CONFIG_IEEE80211AX */

	/*
	 * Add the station with forced WLAN_STA_ASSOC flag. The sta->flags
	 * will be set when the ACK frame for the (Re)Association Response frame
	 * is processed (TX status driver event).
	 */
	if (hostapd_sta_add(hapd, sta->addr, sta->aid, sta->capability,
			    sta->supported_rates, sta->supported_rates_len,
			    sta->listen_interval,
			    sta->flags & WLAN_STA_HT ? &ht_cap : NULL,
			    sta->flags & WLAN_STA_VHT ? &vht_cap : NULL,
			    sta->flags & WLAN_STA_HE ? &he_cap : NULL,
			    sta->flags & WLAN_STA_HE ? sta->he_capab_len : 0,
			    sta->flags | WLAN_STA_ASSOC, sta->qosinfo,
			    sta->vht_opmode, 0,
			    sta->added_unassoc)) {
		hostapd_logger(hapd, sta->addr,
			       HOSTAPD_MODULE_IEEE80211, HOSTAPD_LEVEL_NOTICE,
			       "Could not %s STA to kernel driver",
			       sta->added_unassoc ? "set" : "add");

		if (sta->added_unassoc) {
			hostapd_drv_sta_remove(hapd, sta->addr);
			sta->added_unassoc = 0;
		}

		return -1;
	}

	sta->added_unassoc = 0;

	return 0;
}


static u16 send_assoc_resp(struct hostapd_data *hapd, struct sta_info *sta,
			   u16 status_code, int reassoc, const u8 *ies,
			   size_t ies_len)
{
	int send_len;
	u8 buf[sizeof(struct ieee80211_mgmt) + 1024];
	struct ieee80211_mgmt *reply;
	u8 *p;
#ifdef CONFIG_IEEE80211N
	u8 disable_ht = 0;

    if ((hapd->iconf->bss[0]->wpa_pairwise & WPA_CIPHER_WEP40)  || 
        (hapd->iconf->bss[0]->wpa_pairwise & WPA_CIPHER_WEP104) ||
        (hapd->iconf->bss[0]->wpa_pairwise & WPA_CIPHER_TKIP)   ||
        (hapd->iconf->bss[0]->rsn_pairwise & WPA_CIPHER_TKIP))
	{
	    disable_ht = 1;
	}
#endif

	os_memset(buf, 0, sizeof(buf));
	reply = (struct ieee80211_mgmt *) buf;
	reply->frame_control =
		IEEE80211_FC(WLAN_FC_TYPE_MGMT,
			     (reassoc ? WLAN_FC_STYPE_REASSOC_RESP :
			      WLAN_FC_STYPE_ASSOC_RESP));
	os_memcpy(reply->da, sta->addr, ETH_ALEN);
	os_memcpy(reply->sa, hapd->own_addr, ETH_ALEN);
	os_memcpy(reply->bssid, hapd->own_addr, ETH_ALEN);

	send_len = IEEE80211_HDRLEN;
	send_len += sizeof(reply->u.assoc_resp);
	reply->u.assoc_resp.capab_info =
		host_to_le16(hostapd_own_capab_info(hapd));
	reply->u.assoc_resp.status_code = host_to_le16(status_code);
	reply->u.assoc_resp.aid = host_to_le16(sta->aid | BIT(14) | BIT(15));
	/* Supported rates */
	p = hostapd_eid_supp_rates(hapd, reply->u.assoc_resp.variable);
	/* Extended supported rates */
	p = hostapd_eid_ext_supp_rates(hapd, p);

#ifdef CONFIG_IEEE80211R
	if (status_code == WLAN_STATUS_SUCCESS) {
		/* IEEE 802.11r: Mobility Domain Information, Fast BSS
		 * Transition Information, RSN, [RIC Response] */
		p = wpa_sm_write_assoc_resp_ies(sta->wpa_sm, p,
						buf + sizeof(buf) - p,
						sta->auth_alg, ies, ies_len);
	}
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211W
	if (status_code == WLAN_STATUS_ASSOC_REJECTED_TEMPORARILY)
		p = hostapd_eid_assoc_comeback_time(hapd, sta, p);
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_IEEE80211N
    if (!disable_ht && (sta->flags & WLAN_STA_HT))
    {
    	p = hostapd_eid_ht_capabilities(hapd, p);
    	p = hostapd_eid_ht_operation(hapd, p);
	}
#endif /* CONFIG_IEEE80211N */

#ifdef CONFIG_IEEE80211AC
	if (hapd->iconf->ieee80211ac && !hapd->conf->disable_11ac) {
		u32 nsts = 0, sta_nsts;

		if (hapd->conf->use_sta_nsts && sta->vht_capabilities) {
			struct ieee80211_vht_capabilities *capa;

			nsts = (hapd->iface->conf->vht_capab >>
				VHT_CAP_BEAMFORMEE_STS_OFFSET) & 7;
			capa = sta->vht_capabilities;
			sta_nsts = (le_to_host32(capa->vht_capabilities_info) >>
				    VHT_CAP_BEAMFORMEE_STS_OFFSET) & 7;

			if (nsts < sta_nsts)
				nsts = 0;
			else
				nsts = sta_nsts;
		}
		p = hostapd_eid_vht_capabilities(hapd, p, nsts);
		p = hostapd_eid_vht_operation(hapd, p);
	}
#endif /* CONFIG_IEEE80211AC */

#ifdef CONFIG_IEEE80211AX
	if (hapd->iconf->ieee80211ax && !disable_ht && (sta->flags & WLAN_STA_HE)) {
		p = hostapd_eid_he_capab(hapd, p, IEEE80211_MODE_AP);
		p = hostapd_eid_he_operation(hapd, p);
		p = hostapd_eid_spatial_reuse(hapd, p);
		p = hostapd_eid_he_mu_edca_parameter_set(hapd, p);
	}
#endif /* CONFIG_IEEE80211AX */

	//p = hostapd_eid_ext_capab(hapd, p);
	//p = hostapd_eid_bss_max_idle_period(hapd, p);
	//if (sta->qos_map_enabled)
	//	p = hostapd_eid_qos_map_set(hapd, p);

#ifdef CONFIG_FST
	if (hapd->iface->fst_ies) {
		os_memcpy(p, wpabuf_head(hapd->iface->fst_ies),
			  wpabuf_len(hapd->iface->fst_ies));
		p += wpabuf_len(hapd->iface->fst_ies);
	}
#endif /* CONFIG_FST */

#ifdef CONFIG_IEEE80211AC
	if (hapd->conf->vendor_vht && (sta->flags & WLAN_STA_VENDOR_VHT))
		p = hostapd_eid_vendor_vht(hapd, p);
#endif /* CONFIG_IEEE80211AC */

    if (sta->flags & WLAN_STA_WMM)
        p = hostapd_eid_wmm(hapd, p);

#ifdef CONFIG_WPS
	if ((sta->flags & WLAN_STA_WPS) ||
	    ((sta->flags & WLAN_STA_MAYBE_WPS) && hapd->conf->wpa)) {
		struct wpabuf *wps = wps_build_assoc_resp_ie();
		if (wps) {
			os_memcpy(p, wpabuf_head(wps), wpabuf_len(wps));
			p += wpabuf_len(wps);
			wpabuf_free(wps);
		}
	}
#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
	if (sta->p2p_ie && hapd->p2p_group) {
		struct wpabuf *p2p_resp_ie;
		enum p2p_status_code status;
		switch (status_code) {
		case WLAN_STATUS_SUCCESS:
			status = P2P_SC_SUCCESS;
			break;
		case WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA:
			status = P2P_SC_FAIL_LIMIT_REACHED;
			break;
		default:
			status = P2P_SC_FAIL_INVALID_PARAMS;
			break;
		}
		p2p_resp_ie = p2p_group_assoc_resp_ie(hapd->p2p_group, status);
		if (p2p_resp_ie) {
			os_memcpy(p, wpabuf_head(p2p_resp_ie),
				  wpabuf_len(p2p_resp_ie));
			p += wpabuf_len(p2p_resp_ie);
			wpabuf_free(p2p_resp_ie);
		}
	}
#endif /* CONFIG_P2P */

#ifdef CONFIG_P2P_MANAGER
	if (hapd->conf->p2p & P2P_MANAGE)
		p = hostapd_eid_p2p_manage(hapd, p);
#endif /* CONFIG_P2P_MANAGER */

	//p = hostapd_eid_mbo(hapd, p, buf + sizeof(buf) - p);

	/*if (hapd->conf->assocresp_elements &&
	    (size_t) (buf + sizeof(buf) - p) >=
	    wpabuf_len(hapd->conf->assocresp_elements)) {
		os_memcpy(p, wpabuf_head(hapd->conf->assocresp_elements),
			  wpabuf_len(hapd->conf->assocresp_elements));
		p += wpabuf_len(hapd->conf->assocresp_elements);
	}*/

	send_len += p - reply->u.assoc_resp.variable;

	if (hostapd_drv_send_mlme(hapd, reply, send_len, 0) < 0) {
		wpa_printf(MSG_INFO, "Failed to send assoc resp: %s",
			   strerror(errno));
		return WLAN_STATUS_UNSPECIFIED_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}


static void handle_assoc(struct hostapd_data *hapd,
			 const struct ieee80211_mgmt *mgmt, size_t len,
			 int reassoc)
{
	u16 capab_info, listen_interval, seq_ctrl, fc;
	u16 resp = WLAN_STATUS_SUCCESS, reply_res;
	const u8 *pos;
	int left, i;
	struct sta_info *sta;

	if (len < IEEE80211_HDRLEN + (reassoc ? sizeof(mgmt->u.reassoc_req) :
				      sizeof(mgmt->u.assoc_req))) {
		wpa_printf(MSG_INFO, "handle_assoc(reassoc=%d) - too short payload (len=%lu)",
			   reassoc, (unsigned long) len);
		return;
	}

#ifdef CONFIG_TESTING_OPTIONS
	if (reassoc) {
		if (hapd->iconf->ignore_reassoc_probability > 0.0 &&
		    drand48() < hapd->iconf->ignore_reassoc_probability) {
			wpa_printf(MSG_INFO,
				   "TESTING: ignoring reassoc request from "
				   MACSTR, MAC2STR(mgmt->sa));
			return;
		}
	} else {
		if (hapd->iconf->ignore_assoc_probability > 0.0 &&
		    drand48() < hapd->iconf->ignore_assoc_probability) {
			wpa_printf(MSG_INFO,
				   "TESTING: ignoring assoc request from "
				   MACSTR, MAC2STR(mgmt->sa));
			return;
		}
	}
#endif /* CONFIG_TESTING_OPTIONS */

	fc = le_to_host16(mgmt->frame_control);
	seq_ctrl = le_to_host16(mgmt->seq_ctrl);

	if (reassoc) {
		capab_info = le_to_host16(mgmt->u.reassoc_req.capab_info);
		listen_interval = le_to_host16(
			mgmt->u.reassoc_req.listen_interval);
		wpa_printf(MSG_DEBUG, "reassociation request: STA=" MACSTR
			   " capab_info=0x%02x listen_interval=%d current_ap="
			   MACSTR " seq_ctrl=0x%x%s",
			   MAC2STR(mgmt->sa), capab_info, listen_interval,
			   MAC2STR(mgmt->u.reassoc_req.current_ap),
			   seq_ctrl, (fc & WLAN_FC_RETRY) ? " retry" : "");
		left = len - (IEEE80211_HDRLEN + sizeof(mgmt->u.reassoc_req));
		pos = mgmt->u.reassoc_req.variable;
	} else {
		capab_info = le_to_host16(mgmt->u.assoc_req.capab_info);
		listen_interval = le_to_host16(
			mgmt->u.assoc_req.listen_interval);
		wpa_printf(MSG_DEBUG, "association request: STA=" MACSTR
			   " capab_info=0x%02x listen_interval=%d "
			   "seq_ctrl=0x%x%s",
			   MAC2STR(mgmt->sa), capab_info, listen_interval,
			   seq_ctrl, (fc & WLAN_FC_RETRY) ? " retry" : "");
		left = len - (IEEE80211_HDRLEN + sizeof(mgmt->u.assoc_req));
		pos = mgmt->u.assoc_req.variable;
	}

	sta = ap_get_sta(hapd, mgmt->sa);
#ifdef CONFIG_IEEE80211R
	if (sta && sta->auth_alg == WLAN_AUTH_FT &&
	    (sta->flags & WLAN_STA_AUTH) == 0) {
		wpa_printf(MSG_DEBUG, "FT: Allow STA " MACSTR " to associate "
			   "prior to authentication since it is using "
			   "over-the-DS FT", MAC2STR(mgmt->sa));

		/*
		 * Mark station as authenticated, to avoid adding station
		 * entry in the driver as associated and not authenticated
		 */
		sta->flags |= WLAN_STA_AUTH;
	} else
#endif /* CONFIG_IEEE80211R */
	if (sta == NULL || (sta->flags & WLAN_STA_AUTH) == 0) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "Station tried to "
			       "associate before authentication "
			       "(aid=%d flags=0x%x)",
			       sta ? sta->aid : -1,
			       sta ? sta->flags : 0);
		send_deauth(hapd, mgmt->sa,
			    WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA);
		return;
	}

	if ((fc & WLAN_FC_RETRY) && //here
	    sta->last_seq_ctrl != WLAN_INVALID_MGMT_SEQ &&
	    sta->last_seq_ctrl == seq_ctrl &&
	    sta->last_subtype == (reassoc ? WLAN_FC_STYPE_REASSOC_REQ :
		WLAN_FC_STYPE_ASSOC_REQ)) {  
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "Drop repeated association frame seq_ctrl=0x%x",
			       seq_ctrl);
		return;
	}
	sta->last_seq_ctrl = seq_ctrl;
	sta->last_subtype = reassoc ? WLAN_FC_STYPE_REASSOC_REQ :
		WLAN_FC_STYPE_ASSOC_REQ;

	if (hapd->tkip_countermeasures) {
		resp = WLAN_REASON_MICHAEL_MIC_FAILURE;
		goto fail;
	}

#if 0
	if (listen_interval > hapd->conf->max_listen_interval) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "Too large Listen Interval (%d)",
			       listen_interval);
		resp = WLAN_STATUS_ASSOC_DENIED_LISTEN_INT_TOO_LARGE;
		goto fail;
	}
#endif    

#ifdef CONFIG_MBO
	if (hapd->conf->mbo_enabled && hapd->mbo_assoc_disallow) {
		resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
		goto fail;
	}
#endif /* CONFIG_MBO */

	/*
	 * sta->capability is used in check_assoc_ies() for RRM enabled
	 * capability element.
	 */
	sta->capability = capab_info;

	/* followed by SSID and Supported rates; and HT capabilities if 802.11n
	 * is used */
	resp = check_assoc_ies(hapd, sta, pos, left, reassoc);
	if (resp != WLAN_STATUS_SUCCESS)
		goto fail;

	if (hostapd_get_aid(hapd, sta) < 0) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "No room for more AIDs");
		resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;
		goto fail;
	}

	sta->listen_interval = listen_interval;

	if (hapd->iface->current_mode->mode == HOSTAPD_MODE_IEEE80211G)
		sta->flags |= WLAN_STA_NONERP;
	for (i = 0; i < sta->supported_rates_len; i++) {
		if ((sta->supported_rates[i] & 0x7f) > 22) {
			sta->flags &= ~WLAN_STA_NONERP;
			break;
		}
	}
	if (sta->flags & WLAN_STA_NONERP && !sta->nonerp_set) {
		sta->nonerp_set = 1;
		hapd->iface->num_sta_non_erp++;
		if (hapd->iface->num_sta_non_erp == 1)
			ieee802_11_set_beacons(hapd->iface);
	}

	if (!(sta->capability & WLAN_CAPABILITY_SHORT_SLOT_TIME) &&
	    !sta->no_short_slot_time_set) {
		sta->no_short_slot_time_set = 1;
		hapd->iface->num_sta_no_short_slot_time++;
		if (hapd->iface->current_mode->mode ==
		    HOSTAPD_MODE_IEEE80211G &&
		    hapd->iface->num_sta_no_short_slot_time == 1)
			ieee802_11_set_beacons(hapd->iface);
	}

	if (sta->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
		sta->flags |= WLAN_STA_SHORT_PREAMBLE;
	else
		sta->flags &= ~WLAN_STA_SHORT_PREAMBLE;

	if (!(sta->capability & WLAN_CAPABILITY_SHORT_PREAMBLE) &&
	    !sta->no_short_preamble_set) {
		sta->no_short_preamble_set = 1;
		hapd->iface->num_sta_no_short_preamble++;
		if (hapd->iface->current_mode->mode == HOSTAPD_MODE_IEEE80211G
		    && hapd->iface->num_sta_no_short_preamble == 1)
			ieee802_11_set_beacons(hapd->iface);
	}

#ifdef CONFIG_IEEE80211N
	update_ht_state(hapd, sta);
#endif /* CONFIG_IEEE80211N */

	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG,
		       "association OK (aid %d)", sta->aid);
	/* Station will be marked associated, after it acknowledges AssocResp
	 */
	sta->flags |= WLAN_STA_ASSOC_REQ_OK;

#ifdef CONFIG_IEEE80211W
	if ((sta->flags & WLAN_STA_MFP) && sta->sa_query_timed_out) {
		wpa_printf(MSG_DEBUG, "Allowing %sassociation after timed out "
			   "SA Query procedure", reassoc ? "re" : "");
		/* TODO: Send a protected Disassociate frame to the STA using
		 * the old key and Reason Code "Previous Authentication no
		 * longer valid". Make sure this is only sent protected since
		 * unprotected frame would be received by the STA that is now
		 * trying to associate.
		 */
	}
#endif /* CONFIG_IEEE80211W */

	/* Make sure that the previously registered inactivity timer will not
	 * remove the STA immediately. */
	sta->timeout_next = STA_NULLFUNC;

#ifdef CONFIG_TAXONOMY
	taxonomy_sta_info_assoc_req(hapd, sta, pos, left);
#endif /* CONFIG_TAXONOMY */

 fail:
	/*
	 * In case of a successful response, add the station to the driver.
	 * Otherwise, the kernel may ignore Data frames before we process the
	 * ACK frame (TX status). In case of a failure, this station will be
	 * removed.
	 *
	 * Note that this is not compliant with the IEEE 802.11 standard that
	 * states that a non-AP station should transition into the
	 * authenticated/associated state only after the station acknowledges
	 * the (Re)Association Response frame. However, still do this as:
	 *
	 * 1. In case the station does not acknowledge the (Re)Association
	 *    Response frame, it will be removed.
	 * 2. Data frames will be dropped in the kernel until the station is
	 *    set into authorized state, and there are no significant known
	 *    issues with processing other non-Data Class 3 frames during this
	 *    window.
	 */
	if (resp == WLAN_STATUS_SUCCESS && add_associated_sta(hapd, sta))
		resp = WLAN_STATUS_AP_UNABLE_TO_HANDLE_NEW_STA;

	reply_res = send_assoc_resp(hapd, sta, resp, reassoc, pos, left);

	/*
	 * Remove the station in case tranmission of a success response fails
	 * (the STA was added associated to the driver) or if the station was
	 * previously added unassociated.
	 */
	if ((reply_res != WLAN_STATUS_SUCCESS &&
	     resp == WLAN_STATUS_SUCCESS) || sta->added_unassoc) {
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->added_unassoc = 0;
	}
}


static void handle_disassoc(struct hostapd_data *hapd,
			    const struct ieee80211_mgmt *mgmt, size_t len)
{
	struct sta_info *sta;

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.disassoc)) {
		wpa_printf(MSG_INFO, "handle_disassoc - too short payload (len=%lu)",
			   (unsigned long) len);
		return;
	}

	wpa_printf(MSG_DEBUG, "disassocation: STA=" MACSTR " reason_code=%d",
		   MAC2STR(mgmt->sa),
		   le_to_host16(mgmt->u.disassoc.reason_code));

	sta = ap_get_sta(hapd, mgmt->sa);
	if (sta == NULL) {
		wpa_printf(MSG_INFO, "Station " MACSTR " trying to disassociate, but it is not associated",
			   MAC2STR(mgmt->sa));
		return;
	}

	ap_sta_set_authorized(hapd, sta, 0);
	sta->last_seq_ctrl = WLAN_INVALID_MGMT_SEQ;
	sta->flags &= ~(WLAN_STA_ASSOC | WLAN_STA_ASSOC_REQ_OK);
	wpa_auth_sm_event(sta->wpa_sm, WPA_DISASSOC);
	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO, "disassociated");
	hostapd_drv_sta_remove(hapd, sta->addr);
	sta->added_unassoc = 0;

	if (sta->timeout_next == STA_NULLFUNC ||
	    sta->timeout_next == STA_DISASSOC) {
		sta->timeout_next = STA_DEAUTH;
		eloop_cancel_timeout(ap_handle_timer, hapd, sta);
		eloop_register_timeout(AP_DEAUTH_DELAY, 0, ap_handle_timer,
				       hapd, sta);
	}

	mlme_disassociate_indication(
		hapd, sta, le_to_host16(mgmt->u.disassoc.reason_code));
}


static void handle_deauth(struct hostapd_data *hapd,
			  const struct ieee80211_mgmt *mgmt, size_t len)
{
	struct sta_info *sta;

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.deauth)) {
		wpa_msg(hapd->msg_ctx, MSG_DEBUG, "handle_deauth - too short "
			"payload (len=%lu)", (unsigned long) len);
		return;
	}

	wpa_msg(hapd->msg_ctx, MSG_DEBUG, "deauthentication: STA=" MACSTR
		" reason_code=%d",
		MAC2STR(mgmt->sa), le_to_host16(mgmt->u.deauth.reason_code));

	sta = ap_get_sta(hapd, mgmt->sa);
	if (sta == NULL) {
		wpa_msg(hapd->msg_ctx, MSG_DEBUG, "Station " MACSTR " trying "
			"to deauthenticate, but it is not authenticated",
			MAC2STR(mgmt->sa));
		return;
	}

	ap_sta_set_authorized(hapd, sta, 0);
	sta->last_seq_ctrl = WLAN_INVALID_MGMT_SEQ;
	sta->flags &= ~(WLAN_STA_AUTH | WLAN_STA_ASSOC |
			WLAN_STA_ASSOC_REQ_OK);
	wpa_auth_sm_event(sta->wpa_sm, WPA_DEAUTH);
	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG, "deauthenticated");
	mlme_deauthenticate_indication(
		hapd, sta, le_to_host16(mgmt->u.deauth.reason_code));
	ap_free_sta(hapd, sta);
}


#ifdef COMPILE_WARNING_OPTIMIZE_WPA
static void handle_beacon(struct hostapd_data *hapd,
			  const struct ieee80211_mgmt *mgmt, size_t len,
			  struct hostapd_frame_info *fi)
{
	struct ieee802_11_elems elems;

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.beacon)) {
		wpa_printf(MSG_INFO, "handle_beacon - too short payload (len=%lu)",
			   (unsigned long) len);
		return;
	}

	(void) ieee802_11_parse_elems(mgmt->u.beacon.variable,
				      len - (IEEE80211_HDRLEN +
					     sizeof(mgmt->u.beacon)), &elems,
				      0);

	ap_list_process_beacon(hapd->iface, mgmt, &elems, fi);
}


#ifdef CONFIG_IEEE80211W

static int hostapd_sa_query_action(struct hostapd_data *hapd,
				   const struct ieee80211_mgmt *mgmt,
				   size_t len)
{
	const u8 *end;

	end = mgmt->u.action.u.sa_query_resp.trans_id +
		WLAN_SA_QUERY_TR_ID_LEN;
	if (((u8 *) mgmt) + len < end) {
		wpa_printf(MSG_DEBUG, "IEEE 802.11: Too short SA Query Action "
			   "frame (len=%lu)", (unsigned long) len);
		return 0;
	}

	ieee802_11_sa_query_action(hapd, mgmt->sa,
				   mgmt->u.action.u.sa_query_resp.action,
				   mgmt->u.action.u.sa_query_resp.trans_id);
	return 1;
}


static int robust_action_frame(u8 category)
{
	return category != WLAN_ACTION_PUBLIC &&
		category != WLAN_ACTION_HT;
}
#endif /* CONFIG_IEEE80211W */


static int handle_action(struct hostapd_data *hapd,
			 const struct ieee80211_mgmt *mgmt, size_t len)
{
#if 0
	struct sta_info *sta;
	sta = ap_get_sta(hapd, mgmt->sa);

	if (len < IEEE80211_HDRLEN + 1) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "handle_action - too short payload (len=%lu)",
			       (unsigned long) len);
		return 0;
	}

	if (mgmt->u.action.category != WLAN_ACTION_PUBLIC &&
	    (sta == NULL || !(sta->flags & WLAN_STA_ASSOC))) {
		wpa_printf(MSG_DEBUG, "IEEE 802.11: Ignored Action "
			   "frame (category=%u) from unassociated STA " MACSTR,
			   MAC2STR(mgmt->sa), mgmt->u.action.category);
		return 0;
	}

#ifdef CONFIG_IEEE80211W
	if (sta && (sta->flags & WLAN_STA_MFP) &&
	    !(mgmt->frame_control & host_to_le16(WLAN_FC_ISWEP)) &&
	    robust_action_frame(mgmt->u.action.category)) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "Dropped unprotected Robust Action frame from "
			       "an MFP STA");
		return 0;
	}
#endif /* CONFIG_IEEE80211W */

	if (sta) {
		u16 fc = le_to_host16(mgmt->frame_control);
		u16 seq_ctrl = le_to_host16(mgmt->seq_ctrl);

		if ((fc & WLAN_FC_RETRY) &&
		    sta->last_seq_ctrl != WLAN_INVALID_MGMT_SEQ &&
		    sta->last_seq_ctrl == seq_ctrl &&
		    sta->last_subtype == WLAN_FC_STYPE_ACTION) {
			hostapd_logger(hapd, sta->addr,
				       HOSTAPD_MODULE_IEEE80211,
				       HOSTAPD_LEVEL_DEBUG,
				       "Drop repeated action frame seq_ctrl=0x%x",
				       seq_ctrl);
			return 1;
		}

		sta->last_seq_ctrl = seq_ctrl;
		sta->last_subtype = WLAN_FC_STYPE_ACTION;
	}

	switch (mgmt->u.action.category) {
#ifdef CONFIG_IEEE80211R
	case WLAN_ACTION_FT:
		if (!sta ||
		    wpa_ft_action_rx(sta->wpa_sm, (u8 *) &mgmt->u.action,
				     len - IEEE80211_HDRLEN))
			break;
		return 1;
#endif /* CONFIG_IEEE80211R */
	case WLAN_ACTION_WMM:
		//hostapd_wmm_action(hapd, mgmt, len);
		return 1;
#ifdef CONFIG_IEEE80211W
	case WLAN_ACTION_SA_QUERY:
		return hostapd_sa_query_action(hapd, mgmt, len);
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_WNM
	case WLAN_ACTION_WNM:
		ieee802_11_rx_wnm_action_ap(hapd, mgmt, len);
		return 1;
#endif /* CONFIG_WNM */
#ifdef CONFIG_FST
	case WLAN_ACTION_FST:
		if (hapd->iface->fst)
			fst_rx_action(hapd->iface->fst, mgmt, len);
		else
			wpa_printf(MSG_DEBUG,
				   "FST: Ignore FST Action frame - no FST attached");
		return 1;
#endif /* CONFIG_FST */
	case WLAN_ACTION_PUBLIC:
	case WLAN_ACTION_PROTECTED_DUAL:
#ifdef CONFIG_IEEE80211N
		if (len >= IEEE80211_HDRLEN + 2 &&
		    mgmt->u.action.u.public_action.action ==
		    WLAN_PA_20_40_BSS_COEX) {
			wpa_printf(MSG_DEBUG,
				   "HT20/40 coex mgmt frame received from STA "
				   MACSTR, MAC2STR(mgmt->sa));
			hostapd_2040_coex_action(hapd, mgmt, len);
		}
#endif /* CONFIG_IEEE80211N */
		if (hapd->public_action_cb) {
			hapd->public_action_cb(hapd->public_action_cb_ctx,
					       (u8 *) mgmt, len,
					       hapd->iface->freq);
		}
		if (hapd->public_action_cb2) {
			hapd->public_action_cb2(hapd->public_action_cb2_ctx,
						(u8 *) mgmt, len,
						hapd->iface->freq);
		}
		if (hapd->public_action_cb || hapd->public_action_cb2)
			return 1;
		break;
	case WLAN_ACTION_VENDOR_SPECIFIC:
		if (hapd->vendor_action_cb) {
			if (hapd->vendor_action_cb(hapd->vendor_action_cb_ctx,
						   (u8 *) mgmt, len,
						   hapd->iface->freq) == 0)
				return 1;
		}
		break;
	case WLAN_ACTION_RADIO_MEASUREMENT:
		//hostapd_handle_radio_measurement(hapd, (const u8 *) mgmt, len);
		return 1;
	}

	hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_DEBUG,
		       "handle_action - unknown action category %d or invalid "
		       "frame",
		       mgmt->u.action.category);
	if (!is_multicast_ether_addr(mgmt->da) &&
	    !(mgmt->u.action.category & 0x80) &&
	    !is_multicast_ether_addr(mgmt->sa)) {
		struct ieee80211_mgmt *resp;

		/*
		 * IEEE 802.11-REVma/D9.0 - 7.3.1.11
		 * Return the Action frame to the source without change
		 * except that MSB of the Category set to 1.
		 */
		wpa_printf(MSG_DEBUG, "IEEE 802.11: Return unknown Action "
			   "frame back to sender");
		resp = os_malloc(len);
		if (resp == NULL)
			return 0;
		os_memcpy(resp, mgmt, len);
		os_memcpy(resp->da, resp->sa, ETH_ALEN);
		os_memcpy(resp->sa, hapd->own_addr, ETH_ALEN);
		os_memcpy(resp->bssid, hapd->own_addr, ETH_ALEN);
		resp->u.action.category |= 0x80;

		if (hostapd_drv_send_mlme(hapd, resp, len, 0) < 0) {
			wpa_printf_error(MSG_ERROR, "IEEE 802.11: Failed to send "
				   "Action frame");
		}
		os_free(resp);
	}
#endif
	return 1;
}
#endif /* COMPILE_WARNING_OPTIMIZE_WPA */


/**
 * ieee802_11_mgmt - process incoming IEEE 802.11 management frames
 * @hapd: hostapd BSS data structure (the BSS to which the management frame was
 * sent to)
 * @buf: management frame data (starting from IEEE 802.11 header)
 * @len: length of frame data in octets
 * @fi: meta data about received frame (signal level, etc.)
 *
 * Process all incoming IEEE 802.11 management frames. This will be called for
 * each frame received from the kernel driver through wlan#ap interface. In
 * addition, it can be called to re-inserted pending frames (e.g., when using
 * external RADIUS server as an MAC ACL).
 */
int ieee802_11_mgmt(struct hostapd_data *hapd, const u8 *buf, size_t len,
		    struct hostapd_frame_info *fi)
{
	struct ieee80211_mgmt *mgmt;
	u16 fc, stype;
	int ret = 0;

	if (len < 24)
		return 0;

	mgmt = (struct ieee80211_mgmt *) buf;
	fc = le_to_host16(mgmt->frame_control);
	stype = WLAN_FC_GET_STYPE(fc);

	if (stype == WLAN_FC_STYPE_BEACON) {
		//handle_beacon(hapd, mgmt, len, fi);
		return 1;
	}

	if (!is_broadcast_ether_addr(mgmt->bssid) &&
#ifdef CONFIG_P2P
	    /* Invitation responses can be sent with the peer MAC as BSSID */
	    !((hapd->conf->p2p & P2P_GROUP_OWNER) &&
	      stype == WLAN_FC_STYPE_ACTION) &&
#endif /* CONFIG_P2P */
#ifdef CONFIG_MESH
	    !(hapd->conf->mesh & MESH_ENABLED) &&
#endif /* CONFIG_MESH */
	    os_memcmp(mgmt->bssid, hapd->own_addr, ETH_ALEN) != 0) {
		wpa_printf(MSG_INFO, "MGMT: BSSID=" MACSTR " not our address",
			   MAC2STR(mgmt->bssid));
		return 0;
	}


	if (stype == WLAN_FC_STYPE_PROBE_REQ) {
        //wpa_printf(MSG_DEBUG, "mgmt::probe req");
		handle_probe_req(hapd, mgmt, len, fi->ssi_signal);
		return 1;
	}

	if (os_memcmp(mgmt->da, hapd->own_addr, ETH_ALEN) != 0) {
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "MGMT: DA=" MACSTR " not our address",
			       MAC2STR(mgmt->da));
		return 0;
	}

	//if (hapd->iconf->track_sta_max_num)
	//	sta_track_add(hapd->iface, mgmt->sa);

	switch (stype) {
	case WLAN_FC_STYPE_AUTH:
		wpa_printf(MSG_DEBUG, "mgmt::auth");
		handle_auth(hapd, mgmt, len);
		ret = 1;
		break;
	case WLAN_FC_STYPE_ASSOC_REQ:
		wpa_printf(MSG_DEBUG, "mgmt::assoc_req");
		handle_assoc(hapd, mgmt, len, 0);
		ret = 1;
		break;
	case WLAN_FC_STYPE_REASSOC_REQ:
		wpa_printf(MSG_DEBUG, "mgmt::reassoc_req");
		handle_assoc(hapd, mgmt, len, 1);
		ret = 1;
		break;
	case WLAN_FC_STYPE_DISASSOC:
		wpa_printf(MSG_DEBUG, "mgmt::disassoc");
		handle_disassoc(hapd, mgmt, len);
		ret = 1;
		break;
	case WLAN_FC_STYPE_DEAUTH:
		wpa_msg(hapd->msg_ctx, MSG_DEBUG, "mgmt::deauth");
		handle_deauth(hapd, mgmt, len);
		ret = 1;
		break;
	case WLAN_FC_STYPE_ACTION:
		//wpa_printf(MSG_DEBUG, "mgmt::action");
		//ret = handle_action(hapd, mgmt, len);
		break;
	default:
		hostapd_logger(hapd, mgmt->sa, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "unknown mgmt frame subtype %d", stype);
		break;
	}

	return ret;
}


static void handle_auth_cb(struct hostapd_data *hapd,
			   const struct ieee80211_mgmt *mgmt,
			   size_t len, int ok)
{
	u16 auth_alg, auth_transaction, status_code;
	struct sta_info *sta;

	sta = ap_get_sta(hapd, mgmt->da);
	if (!sta) {
		wpa_printf(MSG_INFO, "handle_auth_cb: STA " MACSTR " not found",
			   MAC2STR(mgmt->da));
		return;
	}

	auth_alg = le_to_host16(mgmt->u.auth.auth_alg);
	auth_transaction = le_to_host16(mgmt->u.auth.auth_transaction);
	status_code = le_to_host16(mgmt->u.auth.status_code);

	if (!ok) {
		hostapd_logger(hapd, mgmt->da, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_NOTICE,
			       "did not acknowledge authentication response");
		goto fail;
	}

	if (len < IEEE80211_HDRLEN + sizeof(mgmt->u.auth)) {
		wpa_printf(MSG_INFO, "handle_auth_cb - too short payload (len=%lu)",
			   (unsigned long) len);
		goto fail;
	}

	if (status_code == WLAN_STATUS_SUCCESS &&
	    ((auth_alg == WLAN_AUTH_OPEN && auth_transaction == 2) ||
	     (auth_alg == WLAN_AUTH_SHARED_KEY && auth_transaction == 4))) {
		hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_INFO, "authenticated");
		sta->flags |= WLAN_STA_AUTH;
		//if (sta->added_unassoc)
		//	hostapd_set_sta_flags(hapd, sta);
		return;
	}

fail:
	if (status_code != WLAN_STATUS_SUCCESS && sta->added_unassoc) {
		hostapd_drv_sta_remove(hapd, sta->addr);
		sta->added_unassoc = 0;
	}
}


#ifdef COMPILE_WARNING_OPTIMIZE_WPA
static void hostapd_set_wds_encryption(struct hostapd_data *hapd,
				       struct sta_info *sta,
				       char *ifname_wds)
{
	int i;
	struct hostapd_ssid *ssid = &hapd->conf->ssid;

	if (hapd->conf->wpa)
		return;

	for (i = 0; i < 4; i++) {
		if (ssid->wep.key[i] &&
		    hostapd_drv_set_key(ifname_wds, hapd, WPA_ALG_WEP, NULL, i,
					i == ssid->wep.idx, NULL, 0,
					ssid->wep.key[i], ssid->wep.len[i])) {
			wpa_printf_warning(MSG_WARNING,
				   "Could not set WEP keys for WDS interface; %s",
				   ifname_wds);
			break;
		}
	}
}
#endif /* COMPILE_WARNING_OPTIMIZE_WPA */


static void handle_assoc_cb(struct hostapd_data *hapd,
			    const struct ieee80211_mgmt *mgmt,
			    size_t len, int reassoc, int ok)
{
	u16 status;
	struct sta_info *sta;
	int new_assoc = 1;

	sta = ap_get_sta(hapd, mgmt->da);
	if (!sta) {
		wpa_printf(MSG_INFO, "handle_assoc_cb: STA " MACSTR " not found",
			   MAC2STR(mgmt->da));
		return;
	}

	if (len < IEEE80211_HDRLEN + (reassoc ? sizeof(mgmt->u.reassoc_resp) :
				      sizeof(mgmt->u.assoc_resp))) {
		wpa_printf(MSG_INFO,
			   "handle_assoc_cb(reassoc=%d) - too short payload (len=%lu)",
			   reassoc, (unsigned long) len);
		hostapd_drv_sta_remove(hapd, sta->addr);
		return;
	}

	if (reassoc)
		status = le_to_host16(mgmt->u.reassoc_resp.status_code);
	else
		status = le_to_host16(mgmt->u.assoc_resp.status_code);

	if (!ok) {
		hostapd_logger(hapd, mgmt->da, HOSTAPD_MODULE_IEEE80211,
			       HOSTAPD_LEVEL_DEBUG,
			       "did not acknowledge association response");
		sta->flags &= ~WLAN_STA_ASSOC_REQ_OK;
		/* The STA is added only in case of SUCCESS */
		if (status == WLAN_STATUS_SUCCESS)
			hostapd_drv_sta_remove(hapd, sta->addr);

		return;
	}

	if (status != WLAN_STATUS_SUCCESS)
		return;

	hostapd_logger(hapd, sta->addr, HOSTAPD_MODULE_IEEE80211,
		       HOSTAPD_LEVEL_INFO,
		       "associated (aid %d)",
		       sta->aid);

	if (sta->flags & WLAN_STA_ASSOC)
		new_assoc = 0;
	sta->flags |= WLAN_STA_ASSOC;
	sta->flags &= ~WLAN_STA_WNM_SLEEP_MODE;
	if ((!hapd->conf->wpa/* && !hapd->conf->osen*/) ||
	    sta->auth_alg == WLAN_AUTH_FT) {
		/*
		 * Open, static WEP, or FT protocol; no separate authorization
		 * step.
		 */
		ap_sta_set_authorized(hapd, sta, 1);
	}

	if (reassoc)
		mlme_reassociate_indication(hapd, sta);
	else
		mlme_associate_indication(hapd, sta);

#ifdef CONFIG_IEEE80211W
	sta->sa_query_timed_out = 0;
#endif /* CONFIG_IEEE80211W */

	/*if (sta->flags & WLAN_STA_WDS) {
		int ret;
		char ifname_wds[IFNAMSIZ + 1];

		ret = hostapd_set_wds_sta(hapd, ifname_wds, sta->addr,
					  sta->aid, 1);
		if (!ret)
			hostapd_set_wds_encryption(hapd, sta, ifname_wds);
	}*/

	//hostapd_set_sta_flags(hapd, sta);

	if (sta->auth_alg == WLAN_AUTH_FT)
		wpa_auth_sm_event(sta->wpa_sm, WPA_ASSOC_FT);
	else
		wpa_auth_sm_event(sta->wpa_sm, WPA_ASSOC);
	hapd->new_assoc_sta_cb(hapd, sta, !new_assoc);

#if 0
	if (sta->pending_eapol_rx) {
		struct os_reltime now, age;

		os_get_reltime(&now);
		os_reltime_sub(&now, &sta->pending_eapol_rx->rx_time, &age);
		if (age.sec == 0 && age.usec < 200000) {
			wpa_printf(MSG_DEBUG,
				   "Process pending EAPOL frame that was received from " MACSTR " just before association notification",
				   MAC2STR(sta->addr));
			/*ieee802_1x_receive(
				hapd, mgmt->da,
				wpabuf_head(sta->pending_eapol_rx->buf),
				wpabuf_len(sta->pending_eapol_rx->buf));*/
		}
		wpabuf_free(sta->pending_eapol_rx->buf);
		os_free(sta->pending_eapol_rx);
		sta->pending_eapol_rx = NULL;
	}
#endif    
}


static void handle_deauth_cb(struct hostapd_data *hapd,
			     const struct ieee80211_mgmt *mgmt,
			     size_t len, int ok)
{
	struct sta_info *sta;
	if (is_multicast_ether_addr(mgmt->da))
		return;
	sta = ap_get_sta(hapd, mgmt->da);
	if (!sta) {
		wpa_printf(MSG_DEBUG, "handle_deauth_cb: STA " MACSTR
			   " not found", MAC2STR(mgmt->da));
		return;
	}
	if (ok)
		wpa_printf(MSG_DEBUG, "STA " MACSTR " acknowledged deauth",
			   MAC2STR(sta->addr));
	else
		wpa_printf(MSG_DEBUG, "STA " MACSTR " did not acknowledge "
			   "deauth", MAC2STR(sta->addr));

	ap_sta_deauth_cb(hapd, sta);
}


static void handle_disassoc_cb(struct hostapd_data *hapd,
			       const struct ieee80211_mgmt *mgmt,
			       size_t len, int ok)
{
	struct sta_info *sta;
	if (is_multicast_ether_addr(mgmt->da))
		return;
	sta = ap_get_sta(hapd, mgmt->da);
	if (!sta) {
		wpa_printf(MSG_DEBUG, "handle_disassoc_cb: STA " MACSTR
			   " not found", MAC2STR(mgmt->da));
		return;
	}
	if (ok)
		wpa_printf(MSG_DEBUG, "STA " MACSTR " acknowledged disassoc",
			   MAC2STR(sta->addr));
	else
		wpa_printf(MSG_DEBUG, "STA " MACSTR " did not acknowledge "
			   "disassoc", MAC2STR(sta->addr));

	ap_sta_disassoc_cb(hapd, sta);
}


/**
 * ieee802_11_mgmt_cb - Process management frame TX status callback
 * @hapd: hostapd BSS data structure (the BSS from which the management frame
 * was sent from)
 * @buf: management frame data (starting from IEEE 802.11 header)
 * @len: length of frame data in octets
 * @stype: management frame subtype from frame control field
 * @ok: Whether the frame was ACK'ed
 */
void ieee802_11_mgmt_cb(struct hostapd_data *hapd, const u8 *buf, size_t len,
			u16 stype, int ok)
{
	const struct ieee80211_mgmt *mgmt;
	mgmt = (const struct ieee80211_mgmt *) buf;

#ifdef CONFIG_TESTING_OPTIONS
	if (hapd->ext_mgmt_frame_handling) {
		wpa_msg(hapd->msg_ctx, MSG_INFO, "MGMT-TX-STATUS stype=%u ok=%d",
			stype, ok);
		return;
	}
#endif /* CONFIG_TESTING_OPTIONS */

	switch (stype) {
	case WLAN_FC_STYPE_AUTH:
		wpa_printf(MSG_DEBUG, "mgmt::auth cb");
		handle_auth_cb(hapd, mgmt, len, ok);
		break;
	case WLAN_FC_STYPE_ASSOC_RESP:
		wpa_printf(MSG_DEBUG, "mgmt::assoc_resp cb");
		handle_assoc_cb(hapd, mgmt, len, 0, ok);
		break;
	case WLAN_FC_STYPE_REASSOC_RESP:
		wpa_printf(MSG_DEBUG, "mgmt::reassoc_resp cb");
		handle_assoc_cb(hapd, mgmt, len, 1, ok);
		break;
	case WLAN_FC_STYPE_PROBE_RESP:
		wpa_printf(MSG_EXCESSIVE, "mgmt::proberesp cb ok=%d", ok);
		break;
	case WLAN_FC_STYPE_DEAUTH:
		wpa_printf(MSG_DEBUG, "mgmt::deauth cb");
		handle_deauth_cb(hapd, mgmt, len, ok);
		break;
	case WLAN_FC_STYPE_DISASSOC:
		wpa_printf(MSG_DEBUG, "mgmt::disassoc cb");
		handle_disassoc_cb(hapd, mgmt, len, ok);
		break;
	case WLAN_FC_STYPE_ACTION:
		wpa_printf(MSG_DEBUG, "mgmt::action cb ok=%d", ok);
		break;
	default:
		wpa_printf(MSG_INFO, "unknown mgmt cb frame subtype %d", stype);
		break;
	}
}


int ieee802_11_get_mib(struct hostapd_data *hapd, char *buf, size_t buflen)
{
	/* TODO */
	return 0;
}


int ieee802_11_get_mib_sta(struct hostapd_data *hapd, struct sta_info *sta,
			   char *buf, size_t buflen)
{
	/* TODO */
	return 0;
}


void hostapd_tx_status(struct hostapd_data *hapd, const u8 *addr,
		       const u8 *buf, size_t len, int ack)
{
	struct sta_info *sta;
	struct hostapd_iface *iface = hapd->iface;

	sta = ap_get_sta(hapd, addr);
	if (sta == NULL && iface->num_bss > 1) {
		size_t j;
		for (j = 0; j < iface->num_bss; j++) {
			hapd = iface->bss[j];
			sta = ap_get_sta(hapd, addr);
			if (sta)
				break;
		}
	}
	if (sta == NULL || !(sta->flags & WLAN_STA_ASSOC))
		return;
	if (sta->flags & WLAN_STA_PENDING_POLL) {
		wpa_printf(MSG_DEBUG, "STA " MACSTR " %s pending "
			   "activity poll", MAC2STR(sta->addr),
			   ack ? "ACKed" : "did not ACK");
		if (ack)
			sta->flags &= ~WLAN_STA_PENDING_POLL;
	}

	//ieee802_1x_tx_status(hapd, sta, buf, len, ack);
}


void hostapd_eapol_tx_status(struct hostapd_data *hapd, const u8 *dst,
			     const u8 *data, size_t len, int ack)
{
	struct sta_info *sta;
	struct hostapd_iface *iface = hapd->iface;

	sta = ap_get_sta(hapd, dst);
	if (sta == NULL && iface->num_bss > 1) {
		size_t j;
		for (j = 0; j < iface->num_bss; j++) {
			hapd = iface->bss[j];
			sta = ap_get_sta(hapd, dst);
			if (sta)
				break;
		}
	}
	if (sta == NULL || !(sta->flags & WLAN_STA_ASSOC)) {
		wpa_printf(MSG_DEBUG, "Ignore TX status for Data frame to STA "
			   MACSTR " that is not currently associated",
			   MAC2STR(dst));
		return;
	}
}


void hostapd_client_poll_ok(struct hostapd_data *hapd, const u8 *addr)
{
	struct sta_info *sta;
	struct hostapd_iface *iface = hapd->iface;

	sta = ap_get_sta(hapd, addr);
	if (sta == NULL && iface->num_bss > 1) {
		size_t j;
		for (j = 0; j < iface->num_bss; j++) {
			hapd = iface->bss[j];
			sta = ap_get_sta(hapd, addr);
			if (sta)
				break;
		}
	}
	if (sta == NULL)
		return;
	wpa_msg(hapd->msg_ctx, MSG_INFO, AP_STA_POLL_OK MACSTR,
		MAC2STR(sta->addr));
	if (!(sta->flags & WLAN_STA_PENDING_POLL))
		return;

	wpa_printf(MSG_DEBUG, "STA " MACSTR " ACKed pending "
		   "activity poll", MAC2STR(sta->addr));
	sta->flags &= ~WLAN_STA_PENDING_POLL;
}


void ieee802_11_rx_from_unknown(struct hostapd_data *hapd, const u8 *src,
				int wds)
{
	struct sta_info *sta;

	sta = ap_get_sta(hapd, src);
	if (sta && (sta->flags & WLAN_STA_ASSOC)) {
        #if 0
		if (!hapd->conf->wds_sta)
			return;

		if (wds && !(sta->flags & WLAN_STA_WDS)) {
			int ret;
			char ifname_wds[IFNAMSIZ + 1];

			wpa_printf(MSG_DEBUG, "Enable 4-address WDS mode for "
				   "STA " MACSTR " (aid %u)",
				   MAC2STR(sta->addr), sta->aid);
			sta->flags |= WLAN_STA_WDS;
			ret = hostapd_set_wds_sta(hapd, ifname_wds,
						  sta->addr, sta->aid, 1);
			if (!ret)
				hostapd_set_wds_encryption(hapd, sta,
							   ifname_wds);
		}
        return;
		#else
            return;
        #endif
	}

	wpa_printf(MSG_DEBUG, "Data/PS-poll frame from not associated STA "
		   MACSTR, MAC2STR(src));
	if (is_multicast_ether_addr(src)) {
		/* Broadcast bit set in SA?! Ignore the frame silently. */
		return;
	}

	if (sta && (sta->flags & WLAN_STA_ASSOC_REQ_OK)) {
		wpa_printf(MSG_DEBUG, "Association Response to the STA has "
			   "already been sent, but no TX status yet known - "
			   "ignore Class 3 frame issue with " MACSTR,
			   MAC2STR(src));
		return;
	}

	if (sta && (sta->flags & WLAN_STA_AUTH))
		hostapd_drv_sta_disassoc(
			hapd, src,
			WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA);
	else
		hostapd_drv_sta_deauth(
			hapd, src,
			WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA);
}


#endif /* CONFIG_NATIVE_WINDOWS */
