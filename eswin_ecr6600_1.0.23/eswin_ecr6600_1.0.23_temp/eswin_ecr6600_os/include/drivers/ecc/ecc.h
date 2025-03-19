
#ifndef DRV_ECC_H
#define DRV_ECC_H


typedef void (*void_ecc_fn)(void);


int drv_ecc256_mod_add(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right);
int drv_ecc256_mod_mult(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right);
int drv_ecc256_mod_sub(unsigned int *p_result, unsigned int *p_left, unsigned int *p_right);
int drv_ecc256_mod_inv(unsigned int *p_result, unsigned int *p_input);

int drv_ecc256_point_mult(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_x, unsigned int *p_point_y, 
							unsigned int *p_scalar);

int drv_ecc256_point_dbl(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_x, unsigned int *p_point_y);

int drv_ecc256_point_add(unsigned int *p_result_x, unsigned int *p_result_y, 
							unsigned int *p_point_left_x, unsigned int *p_point_left_y, 
							unsigned int *p_point_right_x, unsigned int *p_point_right_y);

int drv_ecc256_point_verify(unsigned int *p_point_x, unsigned int *p_point_y);

void drv_ecc256_point_mult_begin(unsigned int *p_point_x, unsigned int *p_point_y, 
							unsigned int *p_scalar,  void_ecc_fn fn);

int drv_ecc256_point_mult_result(unsigned int *p_result_x, unsigned int *p_result_y);

void drv_ecc256_point_mult_abort(void);

#endif /* DRV_ECC_H */

