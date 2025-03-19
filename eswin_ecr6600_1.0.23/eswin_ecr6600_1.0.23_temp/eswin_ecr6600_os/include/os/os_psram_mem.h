#ifndef __OS_PSRAM_MEM_H__
#define __OS_PSRAM_MEM_H__

void os_psram_mem_init(void);
void *os_psram_mem_alloc(uint32_t size);
void *os_psram_mem_free(void *mem_ptr);
void os_psram_mem_read(char *psramptr, char *bufptr, unsigned int len);
void os_psram_mem_write(char *psramptr, char *bufptr, unsigned int len);

#endif