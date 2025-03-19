#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#if 0

//Tuya log print How To Do?
void  PR_ERR(const char *f, ...)
{
	return;
}
void PR_WARN(const char *f, ...)
{
	return;
}
void PR_NOTICE(const char *f, ...)
{
	return;
}
void PR_INFO(const char *f, ...)
{
	return;
}

void PR_DEBUG(const char *f, ...)
{
	return;
}
void LOG_ERR(const char *f, ...)
{}

//WPA 
struct crypto_ec * crypto_ec_init(int group)
{
	return NULL;
}
size_t crypto_ec_prime_len(struct crypto_ec *e)
{
	return 0;
}

const struct crypto_bignum *crypto_ec_get_prime(struct crypto_ec *e)
{
	return NULL;
}

const struct crypto_bignum *crypto_ec_get_order(struct crypto_ec *e)
{
	return NULL;
}

const struct dh_group * dh_groups_get(int id)
{
	return NULL;
}

struct crypto_bignum * crypto_bignum_init_set(const char *buf, size_t len)
{
	return NULL;
}

void crypto_ec_deinit(struct crypto_ec *e)
{
	return;
}

void crypto_bignum_deinit(struct crypto_bignum *n, int clear)
{
	return;
}

void crypto_ec_point_deinit(struct crypto_ec_point *p, int clear)
{
	return;
}

int crypto_bignum_bits(const struct crypto_bignum *a)
{
	return 0;
}
int crypto_bignum_is_zero(const struct crypto_bignum *a)
{
	return 0;
}
int crypto_bignum_is_one(const struct crypto_bignum *a)
{
	return 0;
}

int crypto_bignum_cmp(const struct crypto_bignum *a,
                      const struct crypto_bignum *b)
					  {
						  return 0;
					  }

struct crypto_bignum *crypto_bignum_init(void)
{
	return NULL;
}
int crypto_bignum_mulmod(const struct crypto_bignum *a,
                         const struct crypto_bignum *b,
                         const struct crypto_bignum *c,
                         struct crypto_bignum *d)
						 {
							 return 0;
						 }
int crypto_bignum_legendre(const struct crypto_bignum *a,
                           const struct crypto_bignum *p)
						   {
							   return 0;
						   }
size_t crypto_ec_prime_len_bits(struct crypto_ec *e)
{
	return 0;
}
struct crypto_bignum *crypto_ec_point_compute_y_sqr(struct crypto_ec *e,
		const struct crypto_bignum *x)
		{}		
						   
int crypto_bignum_sub(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
					  {}
int crypto_bignum_div(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
					  {}					  
int crypto_bignum_exptmod(const struct crypto_bignum *a,
                          const struct crypto_bignum *b,
                          const struct crypto_bignum *c,
                          struct crypto_bignum *d)
						  {}

int crypto_bignum_to_bin(const struct crypto_bignum *a,
                         char *buf, size_t buflen, size_t padlen)
						 {}
struct crypto_ec_point *crypto_ec_point_init(struct crypto_ec *e)
{}

int crypto_ec_point_solve_y_coord(struct crypto_ec *e,
		struct crypto_ec_point *p,
		const struct crypto_bignum *x, int y_bit)
		{}
int crypto_ec_point_mul(struct crypto_ec *e, const struct crypto_ec_point *p,
		const struct crypto_bignum *b,
		struct crypto_ec_point *res)
		{}

int crypto_ec_point_invert(struct crypto_ec *e, struct crypto_ec_point *p)
{}


int crypto_bignum_inverse(const struct crypto_bignum *a,
                          const struct crypto_bignum *b,
                          struct crypto_bignum *c)
						  {}

int crypto_bignum_add(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
					  {}

int crypto_ec_point_add(struct crypto_ec *e, const struct crypto_ec_point *a,
		const struct crypto_ec_point *b,
		struct crypto_ec_point *c)
		{}

int crypto_ec_point_is_at_infinity(struct crypto_ec *e,
		const struct crypto_ec_point *p)
		{}


int crypto_bignum_mod(const struct crypto_bignum *a,
                      const struct crypto_bignum *b,
                      struct crypto_bignum *c)
					  {}

int crypto_ec_point_to_bin(struct crypto_ec *e,
		const struct crypto_ec_point *point, char *x, char *y)
		{}

struct crypto_ec_point *crypto_ec_point_from_bin(struct crypto_ec *e,
		const char *val)
		{}

int crypto_ec_point_is_on_curve(struct crypto_ec *e,
		const struct crypto_ec_point *p)
		{}

int crypto_ec_point_cmp(const struct crypto_ec *e,
		const struct crypto_ec_point *a,
		const struct crypto_ec_point *b)
		{}

int trap_l2_pkt(int vif_idx, void *pkt_tmp, int len)
{}

typedef char u8  ;

typedef int sys_err_t ;
//int process_promiscuous_frame(void* rx_head, void* mac_head, uint8_t *frame, uint16_t len)
//{}
//sys_err_t wpa_update_wifi_rssi(int8_t rssi)
//{}
//void * os_memcpy(void *dest, const void *src, size_t n)
//{}

//const u8 * get_ie(const u8 *ies, size_t len, u8 eid)
//{}
//sys_err_t wifi_system_init(void)
//{}
//#define system_event_cb_t void *
//sys_err_t sys_event_loop_init(system_event_cb_t cb, void *ctx)
//{}

//void wpa_supplicant_main(void *env)
//{}


//lwip
void p_buf_link_encapsulation_hlen_too_small(void)
{}

//tuya SDK
#if 0
#include "tuya_hal_wifi.h"
#include "tuya_hal_hostapd.h"
OPERATE_RET tuya_hal_wpas_ap_info_save(unsigned char *ssid, unsigned char *passwd, unsigned char *bssid, unsigned int sec_type)
{
	return 0;
}

OPERATE_RET tuya_hal_wpas_fast_conn_state(unsigned char state)
{
    return 0;
}

//int tuya_os_adapt_wifi_set_work_mode(const WF_WK_MD_E mode)
//{}

#endif

#endif
//c LIB How to do
int _close(int s)
{
	s = s;
	return 0; 
}