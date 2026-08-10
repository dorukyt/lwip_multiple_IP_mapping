/*
 * net_manager.h
 *
 *  Created on: 29 Tem 2026
 *      Author: PC_4434
 */

#ifndef INCLUDE_NET_MANAGER_H_
#define INCLUDE_NET_MANAGER_H_

#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

/* Alias netif ekler. Adresler ip_addr_t (ag bayt sirasi). */
err_t net_if_add(ip_addr_t *ip_addr, ip_addr_t *net_mask, ip_addr_t *gw );

/* Alias netif siler; once soketlerini kaldirir.
 * Fiziksel netif silinemez, ERR_VAL doner. */
err_t net_if_remove(struct netif *netif);

#endif /* INCLUDE_NET_MANAGER_H_ */
