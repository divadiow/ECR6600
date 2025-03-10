#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "FreeRTOS.h"
#include "task.h"
#include "oshal.h"
#include "ecc.h"
#include "pit.h"

#define ECC_LONGS		4

typedef struct _T_ECC_POINT
{
	unsigned long long x[ECC_LONGS];
	unsigned long long y[ECC_LONGS];
} T_ECC_POINT;


int ull_cmp(unsigned long long *p_left, unsigned long long *p_right)
{
	int i;

	for (i = 3; i >= 0; --i)
	{
		if (p_left[i] > p_right[i])
		{
			return 1;
		}
		else if (p_left[i] < p_right[i])
		{
			return -1;
		}
	}

	return 0;
}

//extern void os_hexdump(const void * addr, unsigned int size);

static int test_ecc_mod_add(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned long long data_tmp_ret[ECC_LONGS];

	unsigned long long data_left[ECC_LONGS] = { 0x18191a1b1c1d1e1full, 0x1011121314151617ull, 0x8090a0b0c0d0e0full, 0x001020304050607ull};
	unsigned long long data_right[ECC_LONGS] = { 0x7c4f0440cda5e142ull, 0x12a4b4a6983d7beeull, 0x5f48abec4799fe81ull, 0x8da53a052e187f80ull};
	unsigned long long data_ret[ECC_LONGS] = { 0x94681e5be9c2ff61ull, 0x22b5c6b9ac529205ull, 0x6751b5f753a70c90ull, 0x8da63c08321d8587ull};

	drv_ecc256_mod_add((unsigned int *)data_tmp_ret, (unsigned int *)data_left, (unsigned int *)data_right);

	if (ull_cmp(data_tmp_ret, data_ret) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-add-hardware success\r\n");
	}
	else
	{
		os_printf(LM_CMD,LL_INFO,">> test-mod-add-hardware failed\r\n");
	}


	return 0;
}


CLI_SUBCMD(ut_ecc, ma, test_ecc_mod_add, "unit test ecc mod add", "ut_ecc ma");



static int test_ecc_mod_sub(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned long long data_tmp_ret[ECC_LONGS];

#if 1
	unsigned long long data_left[ECC_LONGS] = { 0x0ull, 0x0ull, 0x0ull, 0x0ull};
	unsigned long long data_right[ECC_LONGS] = { 0xf1860c5be68e887aull, 0xb675b431de3baf9cull, 0x175bdbd866eb53b1ull, 0xcc2943167cffa00cull};
	unsigned long long data_ret[ECC_LONGS] = { 0x0e79f3a419717785ull, 0x498a4bcf21c45063ull, 0xe8a424279914ac4eull, 0x33d6bce883005ff4ull};

#else
	unsigned long long data_left[ECC_LONGS] = { 0x733f0c81a35416e9ull, 0xda3e1f00715554c1ull, 0x693e848a729dd028ull, 0x92e240b08b27320full};
	unsigned long long data_right[ECC_LONGS] = { 0xf4a13945d898c296ull, 0x77037d812deb33a0ull, 0xf8bce6e563a440f2ull, 0x6b17d1f2e12c4247ull};
	unsigned long long data_ret[ECC_LONGS] = { 0x7e9dd33bcabb5453ull, 0x633aa17f436a2120ull, 0x70819da50ef98f36ull, 0x27ca6ebda9faefc7ull};
#endif

	//unsigned long long data_mod[ECC_LONGS] = {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0x0000000000000000ull, 	0xFFFFFFFF00000001ull};

	drv_ecc256_mod_sub((unsigned int *)data_tmp_ret, (unsigned int *)data_left, (unsigned int *)data_right);

	if (ull_cmp(data_tmp_ret, data_ret) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-sub-hardware success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-sub-hardware failed\r\n");
//		os_hexdump((unsigned int *)data_tmp_ret, 32);
	}

	return 0;
}

CLI_SUBCMD(ut_ecc, ms, test_ecc_mod_sub, "unit test ecc mod sub", "ut_ecc ms");



static int test_ecc_mod_mult(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned long long data_tmp_ret[ECC_LONGS];

	unsigned long long data_left[ECC_LONGS] = { 0xd7dcab05cc56f7eaull, 0xd461563e24b30c21ull, 0x50167a0c1e2f3b32ull, 0x864bedf80efed97bull};
	unsigned long long data_ret[ECC_LONGS] = { 0xab8725b67725ab7cull, 0xac95aae474711523ull, 0xe49a53f0832e75a2ull, 0x6e2ee79dcd1028ebull};

	drv_ecc256_mod_mult((unsigned int *)data_tmp_ret, (unsigned int *)data_left, (unsigned int *)data_left);

	if (ull_cmp(data_tmp_ret, data_ret) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-mult-hardware success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-mult-hardware failed\r\n");
	}

	return 0;
}

CLI_SUBCMD(ut_ecc, mm, test_ecc_mod_mult, "unit test ecc mod mult", "ut_ecc mm");

static int test_ecc_mod_inv(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned long long data_tmp_ret[ECC_LONGS];

#if 1

	unsigned long long data_left[ECC_LONGS] = { 0xf1860c5be68e887aull, 0xb675b431de3baf9cull, 0x175bdbd866eb53b1ull, 0xcc2943167cffa00cull};
	unsigned long long data_ret[ECC_LONGS] = { 0x4275b7723ca242b1ull, 0xd633347ca8c16f22ull, 0x4dc322ff944c3bb1ull, 0x153f185a6984e629ull};

#else
	unsigned long long data_left[ECC_LONGS] = { 0x46d2b27d3612c6ffull, 0x68be585549863726ull, 0x8431939ebdbaeb8bull, 0x20ae5921b476c9e9ull};
	unsigned long long data_ret[ECC_LONGS] = { 0x7782e25a03d1746f, 0x7d42704295730667, 0x22ce58b46815af82, 0x6aaee5eb84ffeb6b};
#endif

	//unsigned long long data_mod[ECC_LONGS] = {0xFFFFFFFFFFFFFFFFull, 0x00000000ffffffffull, 0x0000000000000000ull, 0xFFFFFFFF00000001ull};

	drv_ecc256_mod_inv((unsigned int *)data_tmp_ret, (unsigned int *)data_left);
	if (ull_cmp(data_tmp_ret, data_ret) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-inv-hardware success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-mod-inv-hardware failed\r\n");
	}

	return 0;
}
CLI_SUBCMD(ut_ecc, mi, test_ecc_mod_inv, "unit test ecc mod inv", "ut_ecc mi");



static int test_ecc_point_mult(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int time_begin, time_end;

	T_ECC_POINT point_result;	

	unsigned char BasePoint_x_256[32]      = {0x96,0xC2,0x98,0xD8,0x45,0x39,0xA1,0xF4,0xA0,0x33,0xEB,0x2D,0x81,0x7D,0x03,0x77,0xF2,0x40,0xA4,0x63,0xE5,0xE6,0xBC,0xF8,0x47,0x42,0x2C,0xE1,0xF2,0xD1,0x17,0x6B};
	unsigned char BasePoint_y_256[32]      = {0xF5,0x51,0xBF,0x37,0x68,0x40,0xB6,0xCB,0xCE,0x5E,0x31,0x6B,0x57,0x33,0xCE,0x2B,0x16,0x9E,0x0F,0x7C,0x4A,0xEB,0xE7,0x8E,0x9B,0x7F,0x1A,0xFE,0xE2,0x42,0xE3,0x4F};

	unsigned char DebugE256PublicKey_x[32] = 
	{
		0xfa, 0x4d, 0xed, 0x17, 0xdc, 0x5c, 0x0a, 0x0a, 
		0x46, 0x83, 0x14, 0x05, 0xa4, 0xa3, 0x04, 0x22, 
		0x7d, 0x99, 0xe1, 0xab, 0x01, 0x25, 0xf1, 0x7b, 
		0xc2, 0xc6, 0xb7, 0xd7, 0x32, 0x64, 0x51, 0x1f
	};

	unsigned char DebugE256PublicKey_y[32] = 
	{
		0xe8, 0xeb, 0x25, 0x4c, 0x9e, 0x38, 0xe2, 0xd0, 
		0x5f, 0x26, 0xad, 0x57, 0xb7, 0x04, 0xbc, 0x54, 
		0xab, 0xba, 0x9c, 0x46, 0x4b, 0xeb, 0x62, 0x74, 
		0xac, 0xf4, 0x45, 0x20, 0xdf, 0xe5, 0x32, 0xd7
	};

	unsigned char DebugE256SecretKey[32] =   
	{
		0x56, 0x1f, 0xeb, 0x81, 0x77, 0x10, 0xbb, 0x18, 
		0x85, 0x6c, 0x02, 0xcd, 0x4b, 0xae, 0xbc, 0x47, 
		0xa0, 0xf9, 0xb2, 0x70, 0x2a, 0x33, 0x6a, 0xb8, 
		0xdc, 0x37, 0x98, 0x3d, 0x8e, 0x40, 0xbd, 0x23
	};


	//unsigned long  flags = system_irq_save();
	time_begin = drv_pit_get_tick();
	drv_ecc256_point_mult((unsigned int *)point_result.x, (unsigned int *)point_result.y, (unsigned int *)BasePoint_x_256, 
		(unsigned int *)BasePoint_y_256, (unsigned int *)DebugE256SecretKey);
	time_end = drv_pit_get_tick();
	//system_irq_restore(flags);
	
    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware begin-time: %#x\r\n", time_begin);
    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware end-time: %#x\r\n", time_end);
    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware used-time: %d(us)\r\n", (time_end - time_begin)/40);

	if (ull_cmp((unsigned long long *)DebugE256PublicKey_x, point_result.x) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x failed\r\n");
		//os_hexdump((unsigned int *)point_result.x, 32);
	}

	if (ull_cmp((unsigned long long *)DebugE256PublicKey_y, point_result.y) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y failed\r\n");
		//os_hexdump((unsigned int *)point_result.y, 32);
	}

#if 0

	T_ECC_POINT point_result;

#if 1
	T_ECC_POINT point_g =
		{{0x3e4d14a370f8dd04ull, 0xdb13c77b0b8ceedcull, 0x1206288f45772d47ull, 0x230a02db1502de3full}, 
		{0xbd0f6f3379ca7f0aull, 0x8b1cd92941420f15ull, 0x68ff72deb47708d4ull, 0xaca3f9c507141b82ull}};

	T_ECC_POINT point_ret =
		{{0x734323bb9095abdbull, 0x088954b12d68d74aull, 0x7b5c5561a00f9a51ull, 0x6758a1eea98cf8a7ull}, 
		{0xf2e145e3d91794a0ull, 0x97ab60056c7258d1ull, 0xc70638b7543aed27ull, 0xb5d8afa23db6efe0ull}};

	unsigned long long data_scalar[ECC_LONGS] = 
		{ 0x50dfdb7dedd8fee6ull, 0x60af11967a57d1dfull, 0x51eb406ebecb2e7dull, 0x0379ead4386830baull};
#else
	T_ECC_POINT point_g =
		{{0xf4a13945d898c296ull, 0x77037d812deb33a0ull, 0xf8bce6e563a440f2ull, 0x6b17d1f2e12c4247ull}, 
		{0xcbb6406837bf51f5ull, 0x2bce33576b315eceull, 0x8ee7eb4a7c0f9e16ull, 0x4fe342e2fe1a7f9bull}};

	T_ECC_POINT point_ret =
		{{0x29580e9222398732ull, 0xe7a7b70ffe0052c7ull, 0x605bf62f472a3ec6ull, 0xfbfe2bd730871006ull}, 
		{0x00c7b43f93173c9aull, 0x07172a8063e7cd26ull, 0xe9255fe9a7815f65ull, 0x284c56e4a5b1d709ull}};

	unsigned long long data_scalar[ECC_LONGS] = 
		{ 0x708da564183612c6ull, 0x3768be584a930c6eull, 0xb30863274b7b75d7ull, 0x8415cb249b476c9ull};
#endif

	unsigned long long data_mod[ECC_LONGS] = 
		{0xFFFFFFFFFFFFFFFFull, 0x00000000FFFFFFFFull, 0x0000000000000000ull, 0xFFFFFFFF00000001ull};

	drv_ecc256_point_mult((unsigned int *)point_result.x, (unsigned int *)point_result.y, (unsigned int *)point_g.x, 
		(unsigned int *)point_g.y, (unsigned int *)data_scalar, (unsigned int *)data_mod);

	if (ull_cmp(point_ret.x, point_result.x) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x failed\r\n");
	}

	if (ull_cmp(point_ret.y, point_result.y) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y failed\r\n");
	}

#endif
	return 0;
}

CLI_SUBCMD(ut_ecc, pm, test_ecc_point_mult, "unit test ecc point mult", "ut_ecc pm");


static int test_ecc_point_dbl(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int time_begin, time_end;
	T_ECC_POINT point_result;
	T_ECC_POINT point_g =
		{{0xF4A13945D898C296ull, 0x77037D812DEB33A0ull, 0xF8BCE6E563A440F2ull, 0x6B17D1F2E12C4247ull}, 
		{0xCBB6406837BF51F5ull, 0x2BCE33576B315ECEull, 0x8EE7EB4A7C0F9E16ull, 0x4FE342E2FE1A7F9Bull}};

	T_ECC_POINT point_ret =
		{{0xa60b48fc47669978ull, 0xc08969e277f21b35ull, 0x8a52380304b51ac3ull, 0x7cf27b188d034f7eull}, 
		{0x9e04b79d227873d1ull, 0xba7dade63ce98229ull, 0x293d9ac69f7430dbull, 0x07775510db8ed040ull}};

	unsigned long  flags = system_irq_save();
	time_begin = drv_pit_get_tick();
	drv_ecc256_point_dbl((unsigned int *)&point_result.x, (unsigned int *)&point_result.y,
						(unsigned int *)&point_g.x, (unsigned int *)&point_g.y);
	time_end = drv_pit_get_tick();
	system_irq_restore(flags);
	
    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware begin-time: %#x\r\n", time_begin);
    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware end-time: %#x\r\n", time_end);
    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware used-time: %d(us)\r\n", (time_end - time_begin)/40);

	if (ull_cmp(point_ret.x, point_result.x) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware x success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware x failed\r\n");
	}

	if (ull_cmp(point_ret.y, point_result.y) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware y success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-dbl-hardware y failed\r\n");
	}

	return 0;
}
CLI_SUBCMD(ut_ecc, pd, test_ecc_point_dbl, "unit test ecc point dbl", "ut_ecc pd");


static int test_ecc_point_add(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int time_begin, time_end;

	T_ECC_POINT point_result;

	T_ECC_POINT point_left =
		{{0xF4A13945D898C296ull, 0x77037D812DEB33A0ull, 0xF8BCE6E563A440F2ull, 0x6B17D1F2E12C4247ull}, 
		{0xCBB6406837BF51F5ull, 0x2BCE33576B315ECEull, 0x8EE7EB4A7C0F9E16ull, 0x4FE342E2FE1A7F9Bull}};

	T_ECC_POINT point_right =
		{{0xA378828E118F9F75ull, 0xCE359C5C2B8FEEC5ull, 0x3BCA00787FE0FCE7ull, 0x371B42F1304C2FD5ull}, 
		{0xBDBF8358F1551D09ull, 0x785DFD93A6BBFE5Full, 0xB5ED9ED1863AB884ull, 0x7B30E63428F0EE5Bull}};

	T_ECC_POINT point_ret =
		{{0x6C2DDE50C6A2AACAull, 0x9F75D2FF7C023533ull, 0x956EF15452AABB46ull, 0x42A2F7DB5CC83553ull}, 
		{0xECDFA81DC8A878C0ull, 0x18DA6CC1BA5230B1ull, 0x89FB907C0CBB3C9Eull, 0x91EF6A9818A13885ull}};

	unsigned long  flags = system_irq_save();
	time_begin = drv_pit_get_tick();
	drv_ecc256_point_add((unsigned int *)&point_result.x, (unsigned int *)&point_result.y,
						(unsigned int *)&point_left.x, (unsigned int *)&point_left.y,
						(unsigned int *)&point_right.x, (unsigned int *)&point_right.y);
	time_end = drv_pit_get_tick();
	system_irq_restore(flags);
	
    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware begin-time: %#x\r\n", time_begin);
    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware end-time: %#x\r\n", time_end);
    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware used-time: %d(us)\r\n", (time_end - time_begin)/40);

	if (ull_cmp(point_ret.x, point_result.x) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware x success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware x failed\r\n");
	}

	if (ull_cmp(point_ret.y, point_result.y) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware y success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-add-hardware y failed\r\n");
	}

	return 0;
}
CLI_SUBCMD(ut_ecc, pa, test_ecc_point_add, "unit test ecc point add", "ut_ecc pa");


static int test_ecc_point_verify(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret;
	unsigned char BasePoint_x_256[32] = {0x96,0xC2,0x98,0xD8,0x45,0x39,0xA1,0xF4,0xA0,0x33,0xEB,0x2D,0x81,0x7D,0x03,0x77,0xF2,0x40,0xA4,0x63,0xE5,0xE6,0xBC,0xF8,0x47,0x42,0x2C,0xE1,0xF2,0xD1,0x17,0x6B};
	unsigned char BasePoint_y_256[32] = {0xF5,0x51,0xBF,0x37,0x68,0x40,0xB6,0xCB,0xCE,0x5E,0x31,0x6B,0x57,0x33,0xCE,0x2B,0x16,0x9E,0x0F,0x7C,0x4A,0xEB,0xE7,0x8E,0x9B,0x7F,0x1A,0xFE,0xE2,0x42,0xE3,0x4F};
	unsigned int time_begin, time_end;
	
	unsigned long  flags = system_irq_save();
	time_begin = drv_pit_get_tick();
	ret = drv_ecc256_point_verify((unsigned int *)BasePoint_x_256, (unsigned int *)BasePoint_y_256);
	time_end = drv_pit_get_tick();
	system_irq_restore(flags);
	
    	os_printf(LM_CMD,LL_INFO,">> test-point-verify-hardware begin-time: %#x\r\n", time_begin);
    	os_printf(LM_CMD,LL_INFO,">> test-point-verify-hardware end-time: %#x\r\n", time_end);
    	os_printf(LM_CMD,LL_INFO,">> test-point-verify-hardware used-time: %d(us)\r\n", (time_end - time_begin)/40);

	if (ret == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-verify-hardware success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-verify-hardware failed(%d)\r\n", ret);
	}

	return 0;
}
CLI_SUBCMD(ut_ecc, pv, test_ecc_point_verify, "unit test ecc point test_ecc_point_verify", "ut_ecc pv");



void test_eccc_point_mult_callback(void)
{
    	os_printf(LM_CMD,LL_INFO,">> test_eccc_point_mult_callback entry\r\n");
}

static int test_ecc_point_mult_begin(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned char BasePoint_x_256[32]      = {0x96,0xC2,0x98,0xD8,0x45,0x39,0xA1,0xF4,0xA0,0x33,0xEB,0x2D,0x81,0x7D,0x03,0x77,0xF2,0x40,0xA4,0x63,0xE5,0xE6,0xBC,0xF8,0x47,0x42,0x2C,0xE1,0xF2,0xD1,0x17,0x6B};
	unsigned char BasePoint_y_256[32]      = {0xF5,0x51,0xBF,0x37,0x68,0x40,0xB6,0xCB,0xCE,0x5E,0x31,0x6B,0x57,0x33,0xCE,0x2B,0x16,0x9E,0x0F,0x7C,0x4A,0xEB,0xE7,0x8E,0x9B,0x7F,0x1A,0xFE,0xE2,0x42,0xE3,0x4F};

	unsigned char DebugE256SecretKey[32] =   
	{
		0x56, 0x1f, 0xeb, 0x81, 0x77, 0x10, 0xbb, 0x18, 
		0x85, 0x6c, 0x02, 0xcd, 0x4b, 0xae, 0xbc, 0x47, 
		0xa0, 0xf9, 0xb2, 0x70, 0x2a, 0x33, 0x6a, 0xb8, 
		0xdc, 0x37, 0x98, 0x3d, 0x8e, 0x40, 0xbd, 0x23
	};

    	os_printf(LM_CMD,LL_INFO,">> test_ecc_point_mult_begin entry\r\n");


	drv_ecc256_point_mult_begin((unsigned int *)BasePoint_x_256, (unsigned int *)BasePoint_y_256, 
				(unsigned int *)DebugE256SecretKey, test_eccc_point_mult_callback);


	return 0;
}

CLI_SUBCMD(ut_ecc, pmb, test_ecc_point_mult_begin, "unit test ecc point test_ecc_point_mult_begin", "ut_ecc pmb");


static int test_ecc_point_mult_result(cmd_tbl_t *t, int argc, char *argv[])
{
	T_ECC_POINT point_result;	

	unsigned char DebugE256PublicKey_x[32] = 
	{
		0xfa, 0x4d, 0xed, 0x17, 0xdc, 0x5c, 0x0a, 0x0a, 
		0x46, 0x83, 0x14, 0x05, 0xa4, 0xa3, 0x04, 0x22, 
		0x7d, 0x99, 0xe1, 0xab, 0x01, 0x25, 0xf1, 0x7b, 
		0xc2, 0xc6, 0xb7, 0xd7, 0x32, 0x64, 0x51, 0x1f
	};

	unsigned char DebugE256PublicKey_y[32] = 
	{
		0xe8, 0xeb, 0x25, 0x4c, 0x9e, 0x38, 0xe2, 0xd0, 
		0x5f, 0x26, 0xad, 0x57, 0xb7, 0x04, 0xbc, 0x54, 
		0xab, 0xba, 0x9c, 0x46, 0x4b, 0xeb, 0x62, 0x74, 
		0xac, 0xf4, 0x45, 0x20, 0xdf, 0xe5, 0x32, 0xd7
	};

    	os_printf(LM_CMD,LL_INFO,">> test_ecc_point_mult_result entry\r\n");

	drv_ecc256_point_mult_result((unsigned int *)point_result.x, (unsigned int *)point_result.y);

	if (ull_cmp((unsigned long long *)DebugE256PublicKey_x, point_result.x) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware x failed\r\n");
		//os_hexdump((unsigned int *)point_result.x, 32);
	}

	if (ull_cmp((unsigned long long *)DebugE256PublicKey_y, point_result.y) == 0)
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y success\r\n");
	}
	else
	{
	    	os_printf(LM_CMD,LL_INFO,">> test-point-mult-hardware y failed\r\n");
		//os_hexdump((unsigned int *)point_result.y, 32);
	}

	return 0;
}

CLI_SUBCMD(ut_ecc, pmr, test_ecc_point_mult_result, "unit test ecc point test_ecc_point_mult_result", "ut_ecc pmr");



static int test_ecc_point_mult_abort(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned char BasePoint_x_256[32]      = {0x96,0xC2,0x98,0xD8,0x45,0x39,0xA1,0xF4,0xA0,0x33,0xEB,0x2D,0x81,0x7D,0x03,0x77,0xF2,0x40,0xA4,0x63,0xE5,0xE6,0xBC,0xF8,0x47,0x42,0x2C,0xE1,0xF2,0xD1,0x17,0x6B};
	unsigned char BasePoint_y_256[32]      = {0xF5,0x51,0xBF,0x37,0x68,0x40,0xB6,0xCB,0xCE,0x5E,0x31,0x6B,0x57,0x33,0xCE,0x2B,0x16,0x9E,0x0F,0x7C,0x4A,0xEB,0xE7,0x8E,0x9B,0x7F,0x1A,0xFE,0xE2,0x42,0xE3,0x4F};

	unsigned char DebugE256SecretKey[32] =   
	{
		0x56, 0x1f, 0xeb, 0x81, 0x77, 0x10, 0xbb, 0x18, 
		0x85, 0x6c, 0x02, 0xcd, 0x4b, 0xae, 0xbc, 0x47, 
		0xa0, 0xf9, 0xb2, 0x70, 0x2a, 0x33, 0x6a, 0xb8, 
		0xdc, 0x37, 0x98, 0x3d, 0x8e, 0x40, 0xbd, 0x23
	};

    os_printf(LM_CMD,LL_INFO,">> test_ecc_point_mult_abort entry\r\n");
	drv_ecc256_point_mult_begin((unsigned int *)BasePoint_x_256, (unsigned int *)BasePoint_y_256, 
				(unsigned int *)DebugE256SecretKey, test_eccc_point_mult_callback);

	drv_ecc256_point_mult_abort();
	return 0;
}

CLI_SUBCMD(ut_ecc, pma, test_ecc_point_mult_abort, "unit test ecc point test_ecc_point_mult_abort", "ut_ecc pma");



CLI_CMD(ut_ecc, NULL, "unit test ecc", "ut_ecc");

