#include "vnet_filter.h"
#include "oshal.h"

#define VNET_FILTER_RULE_MAX 20

typedef struct _ip_filter
{
    struct _ip_filter *next;
    union {
        vnet_ipv4_filter ipv4Filter;
    } filter;
} vnet_ip_filter_t;

typedef struct
{
    vnet_ip_filter_t *ipv4List;
    vnet_ip_filter_t *ipv4Blist;
    unsigned char default_dir;
} vnet_ipfilter_list;

vnet_ipfilter_list g_ip_list;

static unsigned int vnet_get_list_cnt(unsigned char ip_type)
{
    unsigned int listcnt = 0;
    vnet_ip_filter_t *list = NULL;
    vnet_ip_filter_t *blist = NULL;
    unsigned int flag = system_irq_save();

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        list = g_ip_list.ipv4List;
        blist = g_ip_list.ipv4Blist;
    }

    while (list || blist) {
        if (list != NULL) {
            list = list->next;
            listcnt++;
        }

        if (blist != NULL) {
            blist = blist->next;
            listcnt++;
        }
    }

    system_irq_restore(flag);
    return listcnt;
}

void vnet_ip_filter_clearall(unsigned char ip_type)
{
    vnet_ip_filter_t *list = NULL;
    vnet_ip_filter_t *blist = NULL;
    vnet_ip_filter_t *dnode = NULL;
    unsigned long flag = system_irq_save();

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        list = g_ip_list.ipv4List;
        blist = g_ip_list.ipv4Blist;
    }

    while (list || blist) {
        if (list) {
            dnode = list;
            list = list->next;
            os_free(dnode);
        }

        if (blist) {
            dnode = blist;
            blist = blist->next;
            os_free(dnode);
        }
    }
    g_ip_list.ipv4List = NULL;
    g_ip_list.ipv4Blist = NULL;
    system_irq_restore(flag);
}

int vnet_get_default_filter(void)
{
    return g_ip_list.default_dir;
}

int vnet_set_default_filter(unsigned char dir)
{
    unsigned long flag;
    vnet_ipv4_filter filter;

    if (dir != VNET_PACKET_DICTION_LWIP && dir != VNET_PACKET_DICTION_HOST) {
        os_printf(LM_APP, LL_INFO, "default dir not support %d\n", dir);
        return -1;
    }

    flag = system_irq_save();
    g_ip_list.default_dir = dir;
    system_irq_restore(flag);
    memset(&filter, 0, sizeof(filter));

    filter.dir = VNET_PACKET_DICTION_LWIP;
    filter.mask = VNET_FILTER_MASK_DST_PORT_RANGE;
    filter.dstPmin = 67;
    filter.dstPmax = 68;

    vnet_del_filter(&filter, sizeof(filter), VNET_FILTER_TYPE_IPV4);
    vnet_add_filter(&filter, sizeof(filter), VNET_FILTER_TYPE_IPV4);

    return 0;
}

static int vnet_repeat_filter_check(void *filter, unsigned int len, unsigned char ip_type)
{
    vnet_ip_filter_t *list = NULL;
    vnet_ip_filter_t *blist = NULL;
    unsigned long flag = system_irq_save();

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        list = g_ip_list.ipv4List;
        blist = g_ip_list.ipv4Blist;
    }

    while (list || blist) {
        if (list) {
            if (memcmp(filter, &(list->filter.ipv4Filter), sizeof(vnet_ipv4_filter)) == 0) {
                system_irq_restore(flag);
                os_printf(LM_APP, LL_INFO, "repeat rules error\n");
                return -1;
            }
            list = list->next;
        }

        if (blist) {
            if (memcmp(filter, &(blist->filter.ipv4Filter), sizeof(vnet_ipv4_filter)) == 0) {
                system_irq_restore(flag);
                os_printf(LM_APP, LL_INFO, "repeat rules error\n");
                return -1;
            }
            blist = blist->next;
        }
    }
    system_irq_restore(flag);

    return 0;
}

static int vnet_filter_param_check(void *filter, unsigned char ip_type)
{
    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        vnet_ipv4_filter *ft = (vnet_ipv4_filter *)filter;

        if (ft->mask == 0) {
            os_printf(LM_APP, LL_ERR, "rules mask 0x%x error\n", ft->mask);
            return -1;
        }

        if (ft->dir > VNET_PACKET_DICTION_BOTH) {
            os_printf(LM_APP, LL_ERR, "not support dir %d\n", ft->dir);
            return -1;
        }

        if (ft->mask & VNET_FILTER_MASK_SRC_PORT_RANGE) {
            if (ft->srcPmax <= ft->srcPmin) {
                os_printf(LM_APP, LL_INFO, "sport max(%d) small than min(%d)\n", ft->srcPmax, ft->srcPmin);
                return -1;
            }
        }

        if (ft->mask & VNET_FILTER_MASK_DST_PORT_RANGE) {
            if (ft->dstPmax <= ft->dstPmin) {
                os_printf(LM_APP, LL_INFO, "dport max(%d) small than min(%d)\n", ft->dstPmax, ft->dstPmin);
                return -1;
            }
        }

        if (ft->mask & VNET_FILTER_MASK_PROTOCOL) {
            if (ft->packeType != IPPROTO_UDP && ft->packeType != IPPROTO_TCP) {
                os_printf(LM_APP, LL_INFO, "protocol not support %d\n", ft->packeType);
                return -1;
            }
        }
    }

    return 0;
}

int vnet_add_filter(void *filter, unsigned int len, unsigned char ip_type)
{
    vnet_ip_filter_t *inode = NULL;
    unsigned long flag = 0;

    if (vnet_filter_param_check(filter, ip_type) != 0) {
        return -1;
    }

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        vnet_ipv4_filter *ft = (vnet_ipv4_filter *)filter;

        if (vnet_repeat_filter_check(ft, len, ip_type) < 0) {
            return -1;
        }

        if (vnet_get_list_cnt(VNET_FILTER_TYPE_IPV4) >= VNET_FILTER_RULE_MAX) {
            os_printf(LM_APP, LL_INFO, "rules all ready 0x%x nums\n", vnet_get_list_cnt(VNET_FILTER_TYPE_IPV4));
            return -1;
        }

        inode = (vnet_ip_filter_t *)malloc(sizeof(vnet_ip_filter_t));
        if (inode == NULL) {
            os_printf(LM_APP, LL_INFO, "no buff left\n");
            return -1;
        }
        memset(inode, 0, sizeof(vnet_ip_filter_t));
        memcpy(&inode->filter.ipv4Filter, filter, sizeof(vnet_ipv4_filter));

        flag = system_irq_save();
        if (ft->dir == VNET_PACKET_DICTION_BOTH) {
            inode->next = g_ip_list.ipv4Blist;
            g_ip_list.ipv4Blist = inode;
        } else {
            inode->next = g_ip_list.ipv4List;
            g_ip_list.ipv4List = inode;
        }
    }
    system_irq_restore(flag);

    return 0;
}

int vnet_del_filter(void *filter, unsigned int len, unsigned char ip_type)
{
    vnet_ip_filter_t *list = NULL;
    vnet_ip_filter_t *dnode = NULL;
    unsigned long flag = system_irq_save();

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        vnet_ipv4_filter *ft = (vnet_ipv4_filter *)filter;
        if (ft->dir == VNET_PACKET_DICTION_BOTH) {
            list = g_ip_list.ipv4Blist;
            if (memcmp(&g_ip_list.ipv4Blist->filter.ipv4Filter, ft, sizeof(vnet_ipv4_filter)) == 0) {
                dnode = g_ip_list.ipv4Blist;
                g_ip_list.ipv4Blist = g_ip_list.ipv4Blist->next;
                os_free(dnode);
                system_irq_restore(flag);
                return 0;
            }
        } else {
            list = g_ip_list.ipv4List;
            if (memcmp(&g_ip_list.ipv4List->filter.ipv4Filter, ft, sizeof(vnet_ipv4_filter)) == 0) {
                dnode = g_ip_list.ipv4List;
                g_ip_list.ipv4List = g_ip_list.ipv4List->next;
                os_free(dnode);
                system_irq_restore(flag);
                return 0;
            }
        }

        while (list && list->next) {
            if (memcmp(&list->next->filter.ipv4Filter, ft, sizeof(vnet_ipv4_filter)) == 0) {
                dnode = list->next;
                list->next = list->next->next;
                os_free(dnode);
                system_irq_restore(flag);
                return 0;
            }

            list = list->next;
        }
    }

    system_irq_restore(flag);
    os_printf(LM_APP, LL_INFO, "No match rules to delete\n");
    return -1;
}

int vnet_query_filter(void **filter, unsigned int *len, unsigned char ip_type)
{
    unsigned int cnt = vnet_get_list_cnt(ip_type);
    vnet_ipv4_filter *ft = NULL;
    vnet_ip_filter_t *list = NULL;
    vnet_ip_filter_t *blist = NULL;

    unsigned long flag = system_irq_save();

    if (ip_type == VNET_FILTER_TYPE_IPV4) {
        list = g_ip_list.ipv4List;
        blist = g_ip_list.ipv4Blist;
        *filter = (vnet_ipv4_filter *)malloc(cnt * sizeof(vnet_ipv4_filter));
        if (*filter == NULL) {
            *len = 0;
            system_irq_restore(flag);
            os_printf(LM_APP, LL_INFO, "no buff left to query\n");
            return -1;
        }
        ft = *filter;
    }

    while (list || blist) {
        if (list != NULL) {
            memcpy(ft, &list->filter.ipv4Filter, sizeof(vnet_ipv4_filter));
            ft++;
            list = list->next;
        }

        if (blist != NULL) {
            memcpy(ft, &blist->filter.ipv4Filter, sizeof(vnet_ipv4_filter));
            ft++;
            blist = blist->next;
        }
    }

    system_irq_restore(flag);
    *len = cnt;
    return 0;
}

#define GET_IP(x) (((x)[12] << 24) + ((x)[13] << 16) + ((x)[14] << 8) + (x)[15])
#define GET_PROTO(x) ((x)[9])
#define GET_IPLEN(x) (((x)[0] & 0xF) * 4)
#define GET_SPORT(x) ((((x)[GET_IPLEN(x) + 0]) << 8) + ((x)[GET_IPLEN(x) + 1]))
#define GET_DPORT(x) ((((x)[GET_IPLEN(x) + 2]) << 8) + ((x)[GET_IPLEN(x) + 3]))

static int vnet_ipv4_filter_match(vnet_ip_filter_t *list, unsigned char *packet)
{
    unsigned int ip = GET_IP(packet);
    unsigned char proctol = GET_PROTO(packet);
    unsigned short sport = 0;
    unsigned short dport = 0;
    unsigned long flag = system_irq_save();

    if (proctol == IPPROTO_TCP || proctol == IPPROTO_UDP) {
        sport = GET_SPORT(packet);
        dport = GET_DPORT(packet);
    }

    while (list) {
        if (list->filter.ipv4Filter.mask & VNET_FILTER_MASK_IPADDR) {
            if (ip != list->filter.ipv4Filter.ipaddr) {
                list = list->next;
                continue;
            }
        }

        if (list->filter.ipv4Filter.mask & VNET_FILTER_MASK_PROTOCOL) {
            if (proctol != list->filter.ipv4Filter.packeType) {
                list = list->next;
                continue;
            }
        }

        if ((list->filter.ipv4Filter.mask & VNET_FILTER_MASK_DST_PORT) ||
            (list->filter.ipv4Filter.mask & VNET_FILTER_MASK_DST_PORT_RANGE)) {
            if (dport != list->filter.ipv4Filter.dstPort) {
                if (list->filter.ipv4Filter.dstPmin > dport || list->filter.ipv4Filter.dstPmax < dport) {
                    list = list->next;
                    continue;
                }
            }
        }

        if ((list->filter.ipv4Filter.mask & VNET_FILTER_MASK_SRC_PORT) ||
            (list->filter.ipv4Filter.mask & VNET_FILTER_MASK_SRC_PORT_RANGE)) {
            if (sport != list->filter.ipv4Filter.srcPort) {
                if (list->filter.ipv4Filter.srcPmin > sport || list->filter.ipv4Filter.srcPmax < sport) {
                    list = list->next;
                    continue;
                }
            }
        }

        system_irq_restore(flag);
        return list->filter.ipv4Filter.dir;
    }

    system_irq_restore(flag);
    return g_ip_list.default_dir;
}

int vnet_ipv4_packet_list_filter(unsigned char *packet)
{
    return vnet_ipv4_filter_match(g_ip_list.ipv4List, packet);
}

int vnet_ipv4_packet_blist_filter(unsigned char *packet)
{
    return vnet_ipv4_filter_match(g_ip_list.ipv4Blist, packet);
}

int vnet_filter_wifi_handle(uint16_t type, unsigned char *data)
{
    // ipv4 data packet filter
    if (type != 0x08 || ((*data & 0xf0) >> 4) != 0x04) {
        return -1;
    }

    if (vnet_ipv4_packet_blist_filter(data) == VNET_PACKET_DICTION_BOTH) {
        return -1;
    }

    if (vnet_ipv4_packet_list_filter(data) == VNET_PACKET_DICTION_LWIP) {
        return -1;
    }

    return 0;
}
