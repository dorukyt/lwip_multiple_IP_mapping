/*
 * UDP_source.h
 *
 *  Created on: 14 Tem 2026
 *      Author: DYT
 */

/*
 * UDP soket kayit defteri.
 *
 * Disariya udp_sock_id_t tanitici verilir: ust bayt nesil, alt bayt
 * slot indeksi. Slot her kapatildiginda nesil artar, boylece eski
 * tanitici gecersiz sayilir. Bu yuzden UDP_MAX_LISTENERS 256'yi gecemez.
 *
 * MEMP_NUM_UDP_PCB (lwipopts.h) UDP_MAX_LISTENERS'tan buyuk olmali;
 * pcb havuzu tum yigin tarafindan paylasiliyor.
 */

#ifndef INCLUDE_UDP_SOURCE_H_
#define INCLUDE_UDP_SOURCE_H_

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "lwip/netif.h"

#define UDP_RX_BUF_SIZE 256
#define UDP_MAX_LISTENERS 16

typedef u16_t udp_sock_id_t;
#define UDP_SOCK_ID_INVALID ((udp_sock_id_t)0xFFFF)

typedef struct
{
    ip_addr_t src_ip;
    u16_t src_port;
    u8_t data[UDP_RX_BUF_SIZE];
    u16_t data_len;
    u8_t valid;
} udp_rx_msg_t;

typedef struct
{
    struct udp_pcb *pcb;
    struct netif *netif;
    volatile udp_rx_msg_t rx_msg;
    volatile u32_t rx_count;       /* basariyla okunan paket sayisi */
    volatile u32_t rx_drop_count;  /* kutu doluyken dusen paket sayisi */
    u8_t generation_count;
    u8_t in_use;
} udp_listener_t;

typedef struct
{
    struct netif *netif;
    u16_t local_port;
    udp_sock_id_t socket_id;
} udp_netif_socket_info_t;

/* Tanitici -> local port. Tanitici gecersizse 0. */
u16_t udp_source_get_local_port(udp_sock_id_t socket_id);

/* Netif'in IP'si degistikten SONRA cagrilmali; o netif'teki tum
   soketleri yeni adrese yeniden baglar. */
void udp_source_netif_ip_changed(struct netif *netif);

/* Netif uzerinde verilen portu dinleyen yeni soket acar. */
err_t udp_source_add_listener(struct netif *netif, u16_t local_port,
                              udp_sock_id_t *socket_id);

/* Alim kutusunu okur ve bosaltir. Ana donguden cagrilir. 1 = veri var. */
u8_t udp_source_poll_rx(udp_sock_id_t socket_id, udp_rx_msg_t *msg);

/* Soketi kapatir; slot nesli artar, eski tanitici gecersizlesir. */
void udp_source_remove_listener(udp_sock_id_t socket_id);

/* Belirtilen netif uzerinden gonderir; yonlendirme atlanir. */
err_t udp_source_data_send(udp_sock_id_t socket_id, struct netif *tx_netif,
                    ip_addr_t *ip_addr_rx, u16_t port_number, const u8_t *data,
                    u16_t data_len);

/* Netif'e bagli tum soketleri kapatir. netif silinmeden once cagrilir. */
void udp_source_remove_all_on_netif(struct netif *netif);

/* Netif uzerindeki acik soket sayisi. */
u8_t udp_source_count_sockets(struct netif *netif);

/* Netif'in n'inci soketi (0 tabanli). Yoksa socket_id = UDP_SOCK_ID_INVALID. */
udp_netif_socket_info_t udp_source_get_socket(struct netif *netif, u8_t nth);

#endif /* INCLUDE_UDP_SOURCE_H_ */
