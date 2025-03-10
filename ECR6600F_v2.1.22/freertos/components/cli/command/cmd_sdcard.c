/**
 * @file filename
 * @brief This is a brief description
 * @details This is the detail description
 * @author wuliang
 * @date 2021-9-26
 * @version V0.1
 * @par Copyright by http://eswin.com/.
 * @par History 1:
 *      Date:
 *      Version:
 *      Author:
 *      Modification:
 *
 * @par History 2:
 */


/*--------------------------------------------------------------------------
*												Include files
--------------------------------------------------------------------------*/
#include <stdlib.h>
#include "stdio.h"
#include "stdint.h"
#include "cli.h"
#include "hal_system.h"
#include "sdhci.h"
#include "ff.h"
#include "sdcard.h"



/*--------------------------------------------------------------------------
* 	                                           	Local Macros
--------------------------------------------------------------------------*/
/** Description of the macro */

/*--------------------------------------------------------------------------
* 	                                           	Local Types
--------------------------------------------------------------------------*/
/**
 * @brief The brief description
 * @details The detail description
 */

/*--------------------------------------------------------------------------
* 	                                           	Local Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                           	Local Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Constants
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Global Variables
--------------------------------------------------------------------------*/
/**  Description of global variable  */

/*--------------------------------------------------------------------------
* 	                                          	Global Function Prototypes
--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
* 	                                          	Function Definitions
--------------------------------------------------------------------------*/

FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                      	   /* Open the directory */
    if (res == FR_OK) 
    {
        for (;;) 
        {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) 
            {                    
            												/* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } 
            else 
            {                                      		   /* It is a file. */
                os_printf(LM_CMD, LL_INFO,"%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}


int cmd_sdhci_init(cmd_tbl_t *t, int argc, char *argv[])
{
	sdhci_init();
	return 0;
}

CLI_CMD(sdhci_init, cmd_sdhci_init, "sdhci_init", "sdhci_init");

int cmd_sdcard_init(cmd_tbl_t *t, int argc, char *argv[])
{
    sdcard_init();

    return 0;
}

CLI_CMD(sd_init, cmd_sdcard_init, "sdcard_init", "sdcard_init");

int cmd_sdcard_format(cmd_tbl_t *t, int argc, char *argv[])
{
    static BYTE work[FF_MAX_SS];
    FRESULT fR;

    os_printf(LM_CMD, LL_INFO, "f_mkfs start ...\n");
    fR = f_mkfs("", 0, work, sizeof(work));
    if (fR)
        os_printf(LM_CMD, LL_INFO,"f_mkfs failed\n");
    return 0;
}

CLI_CMD(sd_format, cmd_sdcard_format, "cmd_sdcard_format", "cmd_sdcard_format");


#if 0
int cmd_sdcard_write(cmd_tbl_t *t, int argc, char *argv[])
{
	//sdhci_test_write_and_read(1,0,1000*1000);
	sdhci_test_write_block(1,2,0);
	return 0;
}

CLI_CMD(sd_write, cmd_sdcard_write, "sdcard_write", "sdcard_write");
int cmd_sdcard_read(cmd_tbl_t *t, int argc, char *argv[])
{
	//sdhci_test_write_and_read(2,0,1000*1000);
	sdhci_test_read_block(1,1,0);
	sdhci_test_read_block(1,1,1);
	return 0;
}

CLI_CMD(sd_read, cmd_sdcard_read, "sdcard_read", "sdcard_read");
#endif
int sdcard_file_list(void)
{
	FATFS fs;
    FRESULT res;
    static char buff[256];

    res = f_mount(&fs, "", 1);
    if (res == FR_OK) {
        strcpy(buff, "/");
        res = scan_files(buff);
    }
	f_unmount("");
    return res;
}


int cmd_sdcard_ls(cmd_tbl_t *t, int argc, char *argv[])
{
	
    FATFS fs;
    FRESULT res;
    static char buff[256];


    res = f_mount(&fs, "", 1);
    if (res == FR_OK) {
        strcpy(buff, "/");
        res = scan_files(buff);
    }
	f_unmount("");
    return res;
	//return 0;
}

CLI_CMD(ls, cmd_sdcard_ls, "ls sdcard", "ls sdcard");


int cmd_sdcard_test(cmd_tbl_t *t, int argc, char *argv[])
{
	FATFS myfile;
	FIL file;
	FRESULT fR;
	UINT bytesCnt= 0;
	static BYTE work[FF_MAX_SS];

	os_printf(LM_CMD, LL_INFO,"f_mkfs start ...\n");
	fR = f_mkfs("",0,work,sizeof(work));
	if(fR)
		os_printf(LM_CMD, LL_INFO,"f_mkfs failed\n");
	os_printf(LM_CMD, LL_INFO,"f_mkfs finish\n");
	fR = f_mount(&myfile, "", 1);
	if(fR)
		os_printf(LM_CMD, LL_INFO,"f_mount failed\n");

	fR = f_open(&file, "hello.txt", FA_CREATE_NEW | FA_WRITE);
	if(fR)
		os_printf(LM_CMD, LL_INFO,"f_open failed\n");

	fR = f_write(&file,"Hello, World!\r\n",15,&bytesCnt);
	if(fR)
		os_printf(LM_CMD, LL_INFO,"f_write failed\n");
	os_printf(LM_CMD, LL_INFO,"f_write finish\n");

	f_close(&file);
	f_unmount("");

	sdcard_file_list();

/*
	if(fR == 0)
		{
		os_printf(LM_CMD, LL_INFO,"f_mount success\n");

		
		//if(f_open(&file,(char *)fName, FA_READ|FA_WRITE|FA_CREATE_ALWAYS)== FR_OK)
		if(f_open(&file,(char *)fName, FA_READ)== FR_OK)
			{
			os_printf(LM_CMD, LL_INFO,"f_open success\n");
			//fR = f_write(&file,alaram,sizeof(alaram),&bytesCnt);
			fR = f_read(&file, readbuf, sizeof(readbuf), &bytesCnt);
			os_printf(LM_CMD, LL_INFO,"read buf:%s\n",readbuf);
			f_close(&file);
			//f_mkdir("newdir");
			}
		}
	else
		{
		os_printf(LM_CMD, LL_INFO,"mount failed\n");
		}
*/	
	
	return 0;
}

CLI_CMD(sd_test, cmd_sdcard_test, "sd_test", "sd_test");







