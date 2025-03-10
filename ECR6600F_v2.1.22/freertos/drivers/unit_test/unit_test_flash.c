#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#include "cli.h"
#include "oshal.h"
#include "hal_flash.h"
#include "flash.h"

//#ifndef MIN
//#define MIN(x,y) (x < y ? x : y)
//#endif
//
//#ifndef MAX
//#define MAX(x,y) (x > y ? x : y)
//#endif

#define F_SIZE	1024
//#define F_SIZE	512

#if 0
#define F_SIZE_1		4096
#define cyc_t 		(0x100000/F_SIZE_1)

unsigned char sfTest[F_SIZE_1] = {0};


void xip_stress_test(void)
{	
	unsigned int i,j,ret;
	unsigned int rnum;
	unsigned int rdata;
	unsigned int cycle_time = 0;
	unsigned int error_num = 0;
	
	//DBGLOG_INFO("please entry test time\r\n");
	//cycle_time = utils_ParseData();
	
	for(i = 0; i < F_SIZE_1; i++)
	{
		//sfTest[i] = (i+cycle_time)%256;
		
		sfTest[i] = i;
	}
	
	while(1)
	{
		os_printf(LM_CMD,LL_ERR,"erase all flash memory\r\n");
		
		//ret = drv_spiflash_chiperase();
		
		os_printf(LM_CMD,LL_ERR,"write all flash memory\r\n");
		for(j = 0; j < cyc_t; j++)
		{
		
			ret = drv_spiflash_erase(0x100000 + j*F_SIZE_1,  F_SIZE_1);
			
			if(ret != 0){
				os_printf(LM_CMD,LL_ERR,"drv_spiflash_erase, ret = %d, add = 0x%x\n", ret,0x100000 + j*F_SIZE_1);
				return ;
			}
			ret = drv_spiflash_write(0x100000 + j*F_SIZE_1, (unsigned char *)sfTest, F_SIZE_1);
			if(ret != 0){
				os_printf(LM_CMD,LL_ERR,"drv_spiflash_write, ret = %d, add = 0x%x\n", ret,0x100000 + j*F_SIZE_1);
				return ;
			}
		}
		
		os_printf(LM_CMD,LL_ERR,"xip read data 100000 times \r\n");
		drv_spiflash_read(0x100000, (unsigned char *)&rdata, 4);
		os_printf(LM_CMD,LL_ERR,"test rdata = 0x%x\r\n", rdata);
		for(i = 0; i < 100000; i++)
		{
			//rnum = boot_trng_get();
			
			rnum = rand();
			rnum = rnum % 0x100000;
			drv_spiflash_read(0x100000 + rnum, (unsigned char *)&rdata, 4);
			if((sfTest[rnum%256]) != ((unsigned char)rdata&0xff))
			{ 
				os_printf(LM_CMD,LL_ERR,"!!!! ---------i=%d, rnum = 0x%x, rnum%%256 = 0x%x)\r\n", i, rnum , rnum%256);
				os_printf(LM_CMD,LL_ERR,"!!!! ---------error: rdata = 0x%x, rnum = 0x%x)\r\n", rdata , sfTest[rnum%256]);
				error_num++;
				//return;
			}
		}
		
		//os_printf(LM_CMD,LL_ERR,"0x201228 = %x\r\n",inw(SOC_AON_PAD_SR));
		os_printf(LM_CMD,LL_ERR,"[%d] ok [%d] no\r\n",cycle_time++, error_num);
	}
}

static int stress_tet(cmd_tbl_t *t, int argc, char *argv[])
{
	xip_stress_test();
	return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_flash, stress, stress_tet, "unit test flash erase", "ut_flash erase [address] [length]");
#endif


static int utest_flash_erase(cmd_tbl_t *t, int argc, char *argv[])
{
	int ret = 0;
	unsigned int addr;
	unsigned int length;
	
	if (argc == 3)
	{
		addr = (int)strtoul(argv[1], NULL, 0);
		length = (int)strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash erase [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = hal_flash_erase(addr, length);
	
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"flash erase error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	os_printf(LM_CMD,LL_INFO,"sf_erase, addr = 0x%x,  length = 0x%x\n", addr,length);
	return CMD_RET_SUCCESS;
	
}
CLI_SUBCMD(ut_flash, erase, utest_flash_erase, "unit test flash erase", "ut_flash erase [address] [length]");


static int utest_flash_write(cmd_tbl_t *t, int argc, char *argv[])
{
	int i, ret = 0;
	unsigned int addr;
	unsigned int length;
	unsigned int buff_write;
	unsigned int write_data = 0;
	unsigned int offset = 0;
	unsigned char sfTest[F_SIZE] = {0};

	if (argc >= 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
		buff_write = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash write [address] [length] ([value])\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = check_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, flash write error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	for(i=0; i<F_SIZE; i++){
		sfTest[i] = i;}

	
	if(buff_write != 0)
	{
		memcpy(sfTest,&buff_write,sizeof(buff_write));
	}

	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_write(addr+write_data, (unsigned char *)&sfTest, offset);
		if(ret!=0)
		{
			os_printf(LM_CMD,LL_ERR,"flash write error ,ret =  %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -= offset;
		write_data += offset;
	}
	
	os_printf(LM_CMD,LL_INFO,"sf_program,addr = 0x%x,  length = 0x%x\n", addr,write_data);

	return CMD_RET_SUCCESS;

}
CLI_SUBCMD(ut_flash, write, utest_flash_write, "unit test flash write", "ut_flash write [address] [length]");


static int utest_flash_read(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr;
	unsigned int length;
	unsigned int sf        = 1;
	unsigned int offset    = 0;
    unsigned int read_data = 0;
	int ret;
	unsigned char sf_Test[F_SIZE]  =  {0};

	if (argc == 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash read [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = check_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_read error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD,LL_INFO,"\r\n");

	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_read(addr + read_data, (unsigned char *)sf_Test, offset);
		if(ret != 0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_read error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -=offset;
		read_data += offset;
		
		for(sf = 1; sf <= offset; sf++)
		{
			os_printf(LM_CMD,LL_INFO,"%02x", sf_Test[sf-1]);
			if (sf % 4 == 0)
			{
				os_printf(LM_CMD,LL_INFO," ");
			}
		}
		os_printf(LM_CMD,LL_INFO,"\r\n");
	}
	os_printf(LM_CMD,LL_INFO,"sf_read,addr = 0x%x, length = 0x%x\n", addr,read_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, read, utest_flash_read, "unit test flash read", "ut_flash read [address] [length]");


#if 0
unsigned char sf_Test_[F_SIZE]  __attribute__((section(".dma.data")));

static int utest_flash_read_dma(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr;
	unsigned int length;
	unsigned int sf        = 1;
	unsigned int offset    = 0;
    unsigned int read_data = 0;
	int ret;
	
	if (argc == 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash read [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = check_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_read error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}

	os_printf(LM_CMD,LL_INFO,"\r\n");
	//os_printf(LM_CMD,LL_INFO,"entry   \r\n");

	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = drv_spiflash_read_dma(addr + read_data, (unsigned char *)sf_Test_, offset);

		if(ret != 0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_read error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -=offset;
		read_data += offset;
		
		for(sf = 1; sf <= offset; sf++)
		{
			os_printf(LM_CMD,LL_INFO,"%02x", sf_Test_[sf-1]);
			if (sf % 4 == 0)
			{
				os_printf(LM_CMD,LL_INFO," ");
			}
		}
		os_printf(LM_CMD,LL_INFO,"\r\n");
	}
	os_printf(LM_CMD,LL_INFO,"sf_read,addr = 0x%x, length = 0x%x\n", addr,read_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, read_dma, utest_flash_read_dma, "unit test flash read", "ut_flash read [address] [length]");


static int utest_flash_write_dma(cmd_tbl_t *t, int argc, char *argv[])
{
	int i, ret = 0;
	unsigned int addr;
	unsigned int length;
	unsigned int buff_write;
	unsigned int write_data = 0;
	unsigned int offset = 0;

	if (argc >= 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
		buff_write = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash write [address] [length] ([value])\n", argc);
		return CMD_RET_FAILURE;
	}
	
	ret = check_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, flash write error ,ret =  %d\n", ret);
		return CMD_RET_FAILURE;
	}
	
	for(i=0; i<F_SIZE; i++){
		sf_Test_[i] = i;}

	
	if(buff_write != 0)
	{
		memcpy(sf_Test_,&buff_write,sizeof(buff_write));
	}

	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = drv_spiflash_write_dma(addr+write_data, (unsigned char *)&sf_Test_, offset);
		if(ret!=0)
		{
			os_printf(LM_CMD,LL_ERR,"flash write error ,ret =  %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -= offset;
		write_data += offset;
	}
	
	os_printf(LM_CMD,LL_INFO,"sf_program,addr = 0x%x,  length = 0x%x\n", addr,write_data);

	return CMD_RET_SUCCESS;

}
CLI_SUBCMD(ut_flash, write_dma, utest_flash_write_dma, "unit test flash write", "ut_flash write [address] [length]");

#endif


static int utest_flash_otp_read(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr;
	unsigned int length;
	unsigned int offset = 0;
	unsigned int ret = 0, i=0;
	unsigned int read_data = 0;
	unsigned char sfTest[F_SIZE] = {0};
	
	if (argc == 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash otpread [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}

	ret = check_otp_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_otp_read error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}

	
	memset( sfTest, 0, F_SIZE );
	os_printf(LM_CMD,LL_INFO,"\r\n");

	
	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_otp_read(addr+ read_data, (unsigned char*)(&sfTest[0]), offset);
		if(ret != 0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_otp_read error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		
		for(i=0; i<offset; i++)
		{
			os_printf(LM_CMD,LL_INFO,"[%3x]=0x%02x ",(addr+ read_data+ i),sfTest[i]);
			if((((i+1) % 8) == 0) || (i+1 == offset))
			{
				os_printf(LM_CMD,LL_INFO,"\n");
			}
		}

		length -=offset;
		read_data += offset;
		os_printf(LM_CMD,LL_INFO,"\r\n");
	}

	
	
	
	os_printf(LM_CMD,LL_INFO,"sf_otp read, addr = 0x%x,length = 0x%x\n",addr,read_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, otpread, utest_flash_otp_read, "unit test flash otpread", "ut_flash otpread [address] [length]");


static int utest_flash_otp_write(cmd_tbl_t *t, int argc, char *argv[])
{
 	unsigned int addr;
	unsigned int length;
	unsigned int buff_write;
	unsigned int offset    = 0;
	unsigned int write_data = 0;
	unsigned int ret = 0, i=0;
	unsigned char sfTest[F_SIZE] = {0};
	
	if (argc >= 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
		buff_write = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash otpwrite [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}
	ret = check_otp_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_otp_write error,  ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}


	for(i=0;i<F_SIZE;++i)
	{
		sfTest[i] = i;
	}
	
	for(i=256;i<F_SIZE;++i)
	{
		sfTest[i] = 0x1ff-i;
	}

	for(i=512;i<F_SIZE;++i)
	{
		sfTest[i] = 0x280+i;
	}

	for(i=768;i<F_SIZE;++i)
	{
		sfTest[i] = 0x380-i;
	}
	
	if(buff_write != 0)
	{
		memcpy(sfTest,&buff_write,sizeof(buff_write));
	}
	
	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_otp_write(addr+write_data, (unsigned char *)&sfTest, offset);	
		if(ret!=0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_otp_write error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -= offset;
		write_data += offset;
	}
	
	os_printf(LM_CMD,LL_INFO,"sf_otp_write, addr = 0x%x,length = 0x%x\n",addr,write_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, otpwrite, utest_flash_otp_write, "unit test flash otpwrite", "ut_flash otpwrite [address] [length]([value])");

#if 1
static int utest_flash_otp_erase(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int i = 0;
	//unsigned int length = F_SIZE;
 	unsigned int addr = strtoul(argv[1], NULL, 0);
	unsigned int length = strtoul(argv[2], NULL, 0);
	unsigned int ret = 0;
	unsigned char sfTest[F_SIZE] = {0};
	//ret = hal_flash_otp_erase(addr);
	//ret = spiFlash_OTP_Se(addr);
	//ret = p_flash_dev->flash_otp_ops.flash_otp_erase(p_flash_dev, addr);
	for(i=0;i<length;++i)
	{
		sfTest[i] = 0xff;
	}

	ret = drv_spiflash_OTP_Write(addr, length, (unsigned char *)sfTest);
	
	os_printf(LM_APP, LL_INFO, "sf_otp_erase, ret = %d\n", ret);
	if(ret != 0)	
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, otperase, utest_flash_otp_erase, "unit test flash otperase", "ut_flash otperase [address] )");
#endif


static int utest_flash_otp_read_aes(cmd_tbl_t *t, int argc, char *argv[])
{
	unsigned int addr;
	unsigned int length;
	unsigned int offset = 0;
	unsigned int ret = 0, i=0;
	unsigned int read_data = 0;
	unsigned char sfTest[F_SIZE] = {0};
	
	if (argc == 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash otpread_aes [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}

	ret = check_otp_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_otp_read_aes error, ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}

	
	memset( sfTest, 0, F_SIZE );
	os_printf(LM_CMD,LL_INFO,"\r\n");

	
	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_otp_read_aes(addr+ read_data, (unsigned char*)(&sfTest[0]), offset);
		if(ret != 0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_otp_read_aes error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		
		for(i=0; i<offset; i++)
		{
			os_printf(LM_CMD,LL_INFO,"[%3x]=0x%02x ",(addr+ read_data+ i),sfTest[i]);
			if((((i+1) % 8) == 0) || (i+1 == offset))
			{
				os_printf(LM_CMD,LL_INFO,"\n");
			}
		}

		length -=offset;
		read_data += offset;
		os_printf(LM_CMD,LL_INFO,"\r\n");
	}

	os_printf(LM_CMD,LL_INFO,"sf_otp otpread_aes, addr = 0x%x,length = 0x%x\n",addr,read_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, otpread_aes, utest_flash_otp_read_aes, "unit test flash otpread_aes", "ut_flash otpread_aes [address] [length]");

static int utest_flash_otp_write_aes(cmd_tbl_t *t, int argc, char *argv[])
{
 	unsigned int addr;
	unsigned int length;
	unsigned int buff_write;
	unsigned int offset    = 0;
	unsigned int write_data = 0;
	unsigned int ret = 0, i=0;
	unsigned char sfTest[F_SIZE] = {0};
	
	if (argc >= 3)
	{
		addr = strtoul(argv[1], NULL, 0);
		length = strtoul(argv[2], NULL, 0);
		buff_write = strtoul(argv[3], NULL, 0);
	}
	else
	{
		os_printf(LM_CMD,LL_ERR,"PARAMETER NUM(: %d) IS ERROR!!  ut_flash otpwrite_aes [address] [length]\n", argc);
		return CMD_RET_FAILURE;
	}
	ret = check_otp_oversize(addr,length);
	if(ret != 0)
	{
		os_printf(LM_CMD,LL_ERR,"oversize, sf_otp_write_aes error,  ret = %d\n", ret);
		return CMD_RET_FAILURE;
	}


	for(i=0;i<F_SIZE;++i)
	{
		sfTest[i] = i;
	}
	
	if(buff_write != 0)
	{
		memcpy(sfTest,&buff_write,sizeof(buff_write));
	}
	
	while(length)
	{
	    offset = MIN(F_SIZE, length);
		ret = hal_flash_otp_write_aes(addr+write_data, (unsigned char *)&sfTest, offset);	
		if(ret!=0)
		{
			os_printf(LM_CMD,LL_ERR,"sf_otp_write_aes error, ret = %d\n", ret);
			return CMD_RET_FAILURE;
		}
		length -= offset;
		write_data += offset;
	}
	
	os_printf(LM_CMD,LL_INFO,"sf_otp_write_aes, addr = 0x%x,length = 0x%x\n",addr,write_data);
	return CMD_RET_SUCCESS;
}
CLI_SUBCMD(ut_flash, otpwrite_aes, utest_flash_otp_write_aes, "unit test flash otpwrite_aes", "ut_flash otpwrite_aes [address] [length]([value])");


CLI_CMD(ut_flash, NULL, "unit test flash", "test_flash");


