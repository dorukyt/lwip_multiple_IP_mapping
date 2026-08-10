/*
 * net_manager.c
 *
 *  Created on: 29 Tem 2026
 *      Author: DYT
 */

/*
 * Alias netif ekleme/silme icin ince koordinasyon katmani.
 * Kendi listesini tutmaz; lwiplib'in alias havuzu ile UDP kayit
 * defterini dogru sirada cagirmakla yetinir.
 */

#include "net_manager.h"
#include "lwiplib.h"
#include "UDP_source.h"
#include "lwip/sys.h"

err_t net_if_add(ip_addr_t *ip_addr, ip_addr_t *net_mask, ip_addr_t *gw )
{
    if(NULL == ip_addr || NULL==net_mask || NULL == gw)
        return ERR_VAL;
    /* ip_addr_t ag bayt sirasinda tutar, lwIPAliasAdd host sirasi bekler */
    if(0 == lwIPAliasAdd(0, ntohl(ip_addr->addr), ntohl(net_mask->addr), ntohl(gw->addr)))
        return ERR_MEM;
    return ERR_OK;
}

err_t net_if_remove(struct netif *netif)
{
    if(NULL == netif)
        return ERR_VAL;

    /* sira onemli: once soketler, sonra netif. Ters sirada soket
     * temizligi gecersiz bir netif isaretcisiyle eslesme arar. */
    if(netif == lwIPNetifPtrGet(0))
        return ERR_VAL;

    udp_source_remove_all_on_netif(netif);
    lwIPAliasRemove(netif);

    return ERR_OK;
}
