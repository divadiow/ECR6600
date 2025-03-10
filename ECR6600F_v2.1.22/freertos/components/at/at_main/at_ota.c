#include "at_ota.h"
#include "easyflash.h"
#include "flash.h"
#include "http_ota.h"
#include "ota.h"
#include "oshal.h"
#include "hal_system.h"
#include "httpclient.h"
#include "hal_uart.h"
#include "dce_utils.h"
#ifdef CONFIG_OTA
unsigned int MCU_OTA_ADDR=0x1D8000;    
#define MCU_max_len 0x19000
#if 0
static int TIANGONG_flash_write(unsigned int addr, unsigned int length)
{
	int i, ret = 0;
	unsigned int addr;
	unsigned int length;
	unsigned int buff_write;
	unsigned int write_data = 0;
	unsigned int offset = 0;
	unsigned char sfTest[F_SIZE] = {0};

	
		
		buff_write = strtoul(argv[3], NULL, 0);
	

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
#endif
static int tiangongoffset=0;
static int TIANGONG_http_client_download_data(void *data)
{
    http_event_t *event = data;
    http_client_t *client = event->context;
    int ret;
	//static int tiangongoffset=0;
	
    if (client == NULL) {
        return -1;
    }

    os_printf(LM_APP, LL_DBG, "event type 0x%x len %d totlen %d\n", event->type, event->len, client->total);

    if (event->type == http_event_type_on_headers) {
        if (client->interceptor->response.status != http_response_status_ok) {
            os_printf(LM_APP, LL_ERR, "Err Header status 0x%x\n", client->interceptor->response.status);
            return -1;
        }
    }
	
    if (event->type == http_event_type_on_body) {
        //ret = ota_write((unsigned char *)event->data, event->len);
 
            //os_printf(LM_APP, LL_INFO, "the  data length= %d\n", event->len);
		
         
        ret = drv_spiflash_write(MCU_OTA_ADDR+tiangongoffset, (unsigned char *)event->data, event->len);
		
        if (ret != 0) {
            os_printf(LM_APP, LL_ERR, "write flash failed 0x%x\n", ret);
            return -1;
        }
		tiangongoffset=tiangongoffset+event->len;

		//os_printf(LM_APP, LL_INFO, "the  offset length= %d\n", tiangongoffset);

		
    }

    return 0;
}

int TIANGONG_http_client_download_file(const char *url)
{
    http_client_t *client = http_client_init(NULL);
	
    int ret=0;
		ret= drv_spiflash_erase(MCU_OTA_ADDR, MCU_max_len);
		if (ret != 0) {
            os_printf(LM_APP, LL_ERR, "erase flash failed 0x%x\n", ret);
            return -1;
        }
    ret = http_client_method_request(client, HTTP_REQUEST_METHOD_GET, url, TIANGONG_http_client_download_data);
	if (ret!=0)
		return -1;
	
    http_client_exit(client);
	
    return 0;
}

static dce_result_t dce_handle_CIUPDATE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    if(((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_STRING) /*||
            ((kind & DCE_WRITE) && argc == 2 && 
            argv[0].type == ARG_TYPE_STRING && 
            argv[1].type == ARG_TYPE_NUMBER && 
            (argv[1].value.number == 1 || argv[1].value.number == 0))*/)
    {
        if (SYS_OK == http_client_download_file(argv[0].value.string, strlen(argv[0].value.string)))
        {
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            ota_done(1);
            return DCE_OK;
        }
    }

    ota_done(0);
    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    return DCE_INVALID_INPUT;
}

static dce_result_t dce_handle_CICHANGE(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
    image_headinfo_t imagehead = {0};
    unsigned char image_active_part;
    ota_state_t otastate = {0};
    unsigned int addr;
    unsigned int len;
    unsigned int calcrc;

    if(((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_NUMBER))
    {
        if (argv[0].value.number == 0)
        {
            dce_emit_basic_result_code(dce, DCE_RC_OK); 
            hal_system_reset(RST_TYPE_OTA);
            return DCE_OK;
        }

        if (partion_info_get(PARTION_NAME_CPU, &addr, &len) != 0)
        {
            os_printf(LM_APP, LL_ERR, "cichange can not get %s info\n", PARTION_NAME_CPU);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        // get active part
        extern void *__etext1;
        unsigned int text_addr = ((unsigned int)&__etext1) & 0x1FFFFF;
        os_printf(LM_APP, LL_ERR, "etext1:0x%x\n", text_addr);
        if (text_addr < addr + len / 2)
        {
            addr = addr + len / 2;
            image_active_part = DOWNLOAD_OTA_PARTA;
        }
        else
        {
            image_active_part = DOWNLOAD_OTA_PARTB;
        }

        memset(&imagehead, 0, sizeof(imagehead));
        drv_spiflash_read(addr, (unsigned char *)&imagehead, sizeof(imagehead));
        if (imagehead.update_method != OTA_UPDATE_METHOD_AB)
        {
            os_printf(LM_APP, LL_ERR, "cichange not support method 0x%x\n", imagehead.update_method);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        calcrc = ota_get_flash_crc(addr, offsetof(image_headinfo_t, image_hcrc));
        if (calcrc != imagehead.image_hcrc)
        {
            os_printf(LM_APP, LL_ERR, "cichange calcrc 0x%x hcrc 0x%x\n", calcrc, imagehead.image_hcrc);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }

        if (partion_info_get(PARTION_NAME_OTA_STATUS, &addr, &len) != 0)
        {
            os_printf(LM_APP, LL_ERR, "cichange can not get %s info\n", PARTION_NAME_OTA_STATUS);
            dce_emit_basic_result_code(dce, DCE_RC_ERROR);
            return DCE_FAILED;
        }
        drv_spiflash_read(addr,(unsigned char *)&otastate, sizeof(otastate));

        if (imagehead.image_dcrc != 0)
        {
            calcrc = ota_get_flash_crc(addr, imagehead.data_size + imagehead.xip_size + imagehead.text_size + sizeof(imagehead));
            if (calcrc != imagehead.image_dcrc)
            {
                os_printf(LM_APP, LL_ERR, "cichange calcrc 0x%x dcrc 0x%x\n", calcrc, imagehead.image_dcrc);
                dce_emit_basic_result_code(dce, DCE_RC_ERROR);
                return DCE_FAILED;
            }
        }
        os_printf(LM_APP, LL_ERR, "imageselect.active_part:%x \n",image_active_part);

        if(image_active_part == DOWNLOAD_OTA_PARTB)
        {
            otastate.updata_ab = OTA_AB_UPDATAFLAG;
        }
        else
        { 
            otastate.updata_ab = OTA_AB_UPDATAFLAG | OTA_AB_SELECT_B;
        }
        otastate.update_flag = 1;
        int crclen = offsetof(ota_state_t, patch_addr);
        otastate.crc = ef_calc_crc32(0, (char *)&otastate + crclen, sizeof(otastate) - crclen);
        
        len = len / 2;
        drv_spiflash_erase(addr, len);
        drv_spiflash_write(addr, (unsigned char *)&otastate, sizeof(otastate));
        drv_spiflash_erase(addr + len, len);
        drv_spiflash_write(addr + len, (unsigned char *)&otastate, sizeof(otastate));
        dce_emit_basic_result_code(dce, DCE_RC_OK);
        hal_system_reset(RST_TYPE_OTA);
        return DCE_OK;
    }

    dce_emit_basic_result_code(dce, DCE_RC_ERROR);
    return DCE_FAILED;
}
#if 1
static dce_result_t dce_handle_MCUOTA(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	 if(((kind & DCE_WRITE) && argc == 1 && argv[0].type == ARG_TYPE_STRING) /*||
            ((kind & DCE_WRITE) && argc == 2 && 
            argv[0].type == ARG_TYPE_STRING && 
            argv[1].type == ARG_TYPE_NUMBER && 
            (argv[1].value.number == 1 || argv[1].value.number == 0))*/)
    {

		if (TIANGONG_http_client_download_file(argv[0].value.string)==0)
			{
			//dce_emit_extended_result_code_with_args();
			if(tiangongoffset==0)
				{
				dce_emit_basic_result_code(dce, DCE_RC_ERROR);
				return DCE_FAILED;

				}
			dce_emit_basic_result_code(dce, DCE_RC_OK);
			
			hal_uart_write(1, (unsigned char*)"+MCUOTA:status,", 15);
    		//target_dce_transmit(tiangongoffset, size);
    		 char buf[12];
            size_t str_size;
            dce_itoa(tiangongoffset, buf, sizeof(buf), &str_size);
            hal_uart_write(1,(unsigned char*)buf, str_size);
			
			tiangongoffset=0;
            	return DCE_OK;
			}
		else
			{
				dce_emit_basic_result_code(dce, DCE_RC_ERROR);
				return DCE_FAILED;

		}
		#if 0
        if (SYS_OK == http_client_download_file(argv[0].value.string, strlen(argv[0].value.string)))
        {y
            dce_emit_basic_result_code(dce, DCE_RC_OK);
            ota_done(1);
            return DCE_OK;
        }
		#endif
    }
            return DCE_OK;

}

static dce_result_t dce_handle_OTADATAGET(dce_t* dce, void* group_ctx, int kind, size_t argc, arg_t* argv)
{
	if (argc != 3 || 
		   argv[0].type != ARG_TYPE_NUMBER ||
		   argv[1].type != ARG_TYPE_NUMBER ||
		   argv[2].type != ARG_TYPE_NUMBER ||
		   argv[0].value.number < 0 ||
		   argv[1].value.number < 0 ||
		   argv[2].value.number > MCU_max_len ||
		   argv[2].value.number < 0 
		    ){
		   dce_emit_basic_result_code(dce, DCE_RC_ERROR);
		   return DCE_RC_OK;
	   }
		unsigned int addr=MCU_OTA_ADDR;
    	unsigned int len =  argv[2].value.number;
		
		unsigned char *flash_buff = os_malloc(len);
		unsigned char *a= os_malloc(len*2);;
		memset(flash_buff,0 ,len);
		drv_spiflash_read( addr+argv[1].value.number, flash_buff, len);
		char resp_buf[50];
		memset(resp_buf,0,50);
		hal_uart_write(1,(unsigned char*)"OK\r\n", 4);
		sprintf(resp_buf,"+OTADATAGET:%d,%d,",  argv[1].value.number,argv[2].value.number);

        //dce_emit_information_response(dce, resp_buf, -1);
		hal_uart_write(1,(unsigned char *)resp_buf,strlen(resp_buf));
		for(int i =0 ; i<len;i++)
		{
			
		//os_printf(LM_OS, LL_INFO, "data=%02x\r\n",*((unsigned char *)data+i));
		sprintf((char *)(a+2*i), "%02x",*((unsigned char *)flash_buff+i));
		}

			
		hal_uart_write(1,(unsigned char*)"\"", 1);
		//for(int i =0 ; i<len;i++)
		//{
			
		//os_printf(LM_OS, LL_INFO, "data=%02x\r\n",*((unsigned char *)data+i));
		//hal_uart_write(1, flash_buff,len);
		//}
		hal_uart_write(1,  a, len*2);
		hal_uart_write(1,(unsigned char*)"\"", 1);
		
		hal_uart_write(1,(unsigned char*)"\r\n##\r\n", 6);
		
		os_free(flash_buff);
		os_free(a);
		return DCE_OK;
}

#endif
static const command_desc_t UPDATA_commands[] = {
    {"CIUPDATE"   , &dce_handle_CIUPDATE,    DCE_TEST| DCE_WRITE | DCE_READ},
    {"CICHANGE"   , &dce_handle_CICHANGE,    DCE_TEST| DCE_WRITE | DCE_READ},
    {"MCUOTA"	  , &dce_handle_MCUOTA, 	DCE_TEST| DCE_WRITE | DCE_READ},
    {"OTADATAGET" ,	&dce_handle_OTADATAGET	,DCE_TEST| DCE_WRITE | DCE_READ},
};
#endif

void dce_register_ota_commands(dce_t* dce)
{
#ifdef CONFIG_OTA
    dce_register_command_group(dce, "OTA", UPDATA_commands, sizeof(UPDATA_commands) / sizeof(command_desc_t), 0);
#endif
}



