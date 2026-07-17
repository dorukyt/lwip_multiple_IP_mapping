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

#define UDP_RX_BUF_SIZE 256

typedef struct
{
    ip_addr_t src_ip;
    u16_t src_port;
    u8_t data[UDP_RX_BUF_SIZE];
    u16_t data_len;
    u8_t valid;
} udp_rx_msg_t;

u8_t udp_source_poll_rx(udp_rx_msg_t *msg);

err_t udp_source_init(u16_t listen_port);

void udp_source_deinit(void);

err_t udp_data_send(ip_addr_t *ip_addr_tx ,ip_addr_t *ip_addr_rx , u16_t port_number , const u8_t *data , u16_t data_len);

#endif /* INCLUDE_UDP_SOURCE_H_ */
