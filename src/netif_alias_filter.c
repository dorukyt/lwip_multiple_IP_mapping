/**
 * @file netif_alias_filter.c
 * @brief Virtual IP'ler icin gelen paketlerin dogru netif'e yonlendirilmesi.
 */

#include "netif_alias_filter.h"
#include "lwip/opt.h"
#include "lwip/ip.h"
#include "netif/etharp.h"
#include <string.h>

struct netif* netif_alias_route_filter(struct pbuf *p, struct netif *def_netif, u16_t type)
{
    struct netif *n;
    ip_addr_t dest_ip;
    u8_t target_ip_found = 0;

    /* Guvenlik kontrolu: Bellek tahsis edilememis veya netif bos olabilir */
    if (p == NULL || def_netif == NULL) {
        return def_netif;
    }

    /* 1. Paketin icerisinden Hedef (Destination) IP adresini cikaralim */
    if (type == ETHTYPE_IP) {
        /* IPv4 Paketi */
        /* Ethernet basligini (14 byte) atlayip IP basligina ulasmamiz gerekiyor.
           Guvenlik amaciyla pbuf boyutunu kontrol ediyoruz. */
        if (p->len >= (SIZEOF_ETH_HDR + IP_HLEN)) {
            /* Data Abort tuzagina dusmemek icin pointer'i byte bazli (u8_t*) atliyoruz. */
            struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + SIZEOF_ETH_HDR);

            /* lwIP'nin guvenli kopyalama makrosu ile IP'yi aliyoruz */
            ip_addr_copy(dest_ip, iphdr->dest);
            target_ip_found = 1;
        }
    }
    else if (type == ETHTYPE_ARP) {
        /* ARP Paketi */
        if (p->len >= (SIZEOF_ETH_HDR + sizeof(struct etharp_hdr))) {
            struct etharp_hdr *arphdr = (struct etharp_hdr *)((u8_t *)p->payload + SIZEOF_ETH_HDR);

            /* ARP paketinin hedef IP'sini (dipaddr) aliyoruz.
               Yine Data Abort riskine karsi memcpy ile risksiz kopyalama yapiyoruz. */
            memcpy(&dest_ip, &arphdr->dipaddr, sizeof(ip_addr_t));
            target_ip_found = 1;
        }
    }

    /* Eger paket IP veya ARP degilse (ornek: IPv6), varsayilan netif yonetsin. */
    if (!target_ip_found) {
        return def_netif;
    }

    /* 2. Performans Optimizasyonu: Once default netif'i kontrol et.
       Ag trafiginin %90'i genelde ana IP'ye gelir. Donguye girmeden cikmak islemciyi rahatlatir. */
    if (ip_addr_cmp(&dest_ip, &(def_netif->ip_addr))) {
        return def_netif;
    }

    /* 3. Broadcast ve Multicast Kontrolu
       Bu paketler tum ag icin gecerlidir, ayrica sanal netif'e yonlendirmeye gerek yoktur. */
    if (ip_addr_isbroadcast(&dest_ip, def_netif) || ip_addr_ismulticast(&dest_ip)) {
        return def_netif;
    }

    /* 4. Eslesme Arama: Default degilse, sistemdeki diger netif'leri tarayalim (Virtual IP'ler) */
    for (n = netif_list; n != NULL; n = n->next) {
        /* netif aktif mi (UP ve LINK_UP) ve IP adresi uyusuyor mu? */
        if (netif_is_up(n) && (n->flags & NETIF_FLAG_LINK_UP)) {
            if (ip_addr_cmp(&dest_ip, &(n->ip_addr))) {
                /* Eslesme bulundu! Paketi lwIP'ye bu sanal netif uzerinden veriyoruz. */
                return n;
            }
        }
    }

    /* 5. Gecersiz Hedef: Hicbir IP'ye uymuyorsa paketi default netif'te birak,
       lwIP'nin alt katmanlari (ip_input) bu yabanci paketi Drop edecektir (cope atacaktir). */
    return def_netif;
}
