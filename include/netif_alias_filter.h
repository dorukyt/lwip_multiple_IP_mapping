/**
 * @file netif_alias_filter.h
 * @brief lwIP icin tek donanimda (EMAC) coklu IP (Aliasing) filtreleme modulu.
 */

#ifndef NETIF_ALIAS_FILTER_H
#define NETIF_ALIAS_FILTER_H

#include "lwip/netif.h"
#include "lwip/pbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gelen paketin Hedef IP adresine bakarak dogru netif'i dondurur.
 * * @param p Gelen paketi barindiran pbuf yapisi (Ethernet basligi ile birlikte)
 * @param def_netif Donanimin bagli oldugu varsayilan (default) netif
 * @param type Ethernet paketinin tipi (ETHTYPE_IP, ETHTYPE_ARP vs.)
 * @return struct netif* Paketin yonlendirilecegi dogru netif
 */
struct netif* netif_alias_route_filter(struct pbuf *p, struct netif *def_netif, u16_t type);

#ifdef __cplusplus
}
#endif

#endif /* NETIF_ALIAS_FILTER_H */
