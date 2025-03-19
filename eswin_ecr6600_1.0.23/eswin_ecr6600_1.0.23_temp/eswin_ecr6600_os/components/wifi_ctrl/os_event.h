#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#define OS_EVENT_STA_GOT_IP (0x1 << 0)
#define OS_EVENT_STA_REMOVE_NETWORK (0x1 << 1)


