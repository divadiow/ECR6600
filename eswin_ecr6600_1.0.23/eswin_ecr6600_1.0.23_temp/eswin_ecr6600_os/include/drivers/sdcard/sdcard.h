

#include <stdio.h>

#include "ff.h"


int sdcard_init(void);
int sdcard_write(const BYTE *buff,LBA_t sector,UINT count);
int sdcard_read(const BYTE *buff,LBA_t sector,UINT count);
unsigned int sdcard_get_blk_size(void);
unsigned int sdcard_get_sector_count(void);
int sdcard_status(void);
unsigned int sdcard_get_ccs(void);










