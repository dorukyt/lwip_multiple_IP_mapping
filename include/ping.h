/*
 * ping.h
 */

#ifndef INCLUDE_PING_H_
#define INCLUDE_PING_H_

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

/* icmp payload bytes carried in every echo request */
#define PING_DATA_SIZE      32

/* defaults used by the terminal ping command */
#define PING_ATTEMPT_COUNT  4
#define PING_TIMEOUT_MS     2000

typedef struct
{
    ip_addr_t from;     /* replying host */
    u16_t seqno;        /* sequence number of the reply */
    u16_t data_len;     /* icmp payload length of the reply */
    u32_t rtt_ms;       /* round trip time in milliseconds */
    u8_t ttl;           /* ip ttl of the reply */
} ping_result_t;

/* sends one ICMP echo request out of the given netif (source IP and
 * gateway of that netif are forced, like udp_sendto_if for UDP) */
err_t ping_send(struct netif *netif, ip_addr_t *target);

/* busy-waits until the reply of the last ping_send arrives or timeout_ms
 * passes; returns 1 and fills *out on success, 0 on timeout */
u8_t ping_wait_reply(u32_t timeout_ms, ping_result_t *out);

//non blocking wait for the given amount of time
static void delay_ms(uint32_t ms);

//scans the whole network with ARP request to find out network
err_t scan_network(struct netif *n);

#endif /* INCLUDE_PING_H_ */
