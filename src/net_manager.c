/*
 * net_manager.c
 *
 *  Created on: 29 Tem 2026
 *      Author: PC_4434
 */

#include "net_manager.h"
#include "lwiplib.h"
#include "UDP_source.h"
#include "lwip/sys.h"

err_t net_if_add(ip_addr_t *ip_addr, ip_addr_t *net_mask, ip_addr_t *gw )
{
    if(NULL == ip_addr || NULL==net_mask || NULL == gw)
        return ERR_VAL;
    if(0 == lwIPAliasAdd(0, ntohl(ip_addr->addr), ntohl(net_mask->addr), ntohl(gw->addr)))
        return ERR_MEM;
    return ERR_OK;
}

err_t net_if_remove(struct netif *netif)
{
    if(NULL == netif)
        return ERR_VAL;

    //block if user tries to delete the default netif
    if(netif == lwIPNetifPtrGet(0))
        return ERR_VAL;

    udp_source_remove_all_on_netif(netif);
    lwIPAliasRemove(netif);

    return ERR_OK;
}
