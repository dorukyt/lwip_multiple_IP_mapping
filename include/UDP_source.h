/*
 * UDP_source.h
 *
 *  Created on: 14 Tem 2026
 *      Author: PC_4434
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

#define UDP_MAX_LISTENERS 4

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
    volatile udp_rx_msg_t rx_msg;
    volatile u32_t rx_count;
    volatile u32_t rx_drop_count;
    u8_t in_use;
}udp_listener_t;

u16_t udp_source_get_local_port(u8_t handle);

err_t udp_source_add_listener(ip_addr_t *local_ip, u16_t local_port, u8_t *handle_out);

u8_t udp_source_poll_rx(u8_t handle, udp_rx_msg_t *msg);

void udp_source_remove_listener(u8_t handle);

err_t udp_data_send(u8_t handle, struct netif *tx_netif, ip_addr_t *ip_addr_rx,
                    u16_t port_number, const u8_t *data, u16_t data_len);

#endif /* INCLUDE_UDP_SOURCE_H_ */
