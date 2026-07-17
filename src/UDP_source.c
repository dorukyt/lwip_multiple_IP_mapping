/*
 * UDP_source.c
 *
 *  Created on: 14 Tem 2026
 *      Author: PC_4434
 */

#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "UDP_source.h"

#include <string.h>

static struct udp_pcb *udp_pcb = NULL;

static volatile udp_rx_msg_t s_rx_msg;

static volatile u32_t s_rx_count = 0;
static volatile u32_t s_rx_drop_count = 0;

static void udp_rx_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            ip_addr_t *addr, u16_t port);

static u32_t byte_swap32(u32_t v)
{
    return ((v & 0x000000FFUL) << 24) |
           ((v & 0x0000FF00UL) << 8)  |
           ((v & 0x00FF0000UL) >> 8)  |
           ((v & 0xFF000000UL) >> 24);
}


err_t udp_source_init(u16_t listen_port){

    if(udp_pcb != NULL){
        udp_remove(udp_pcb);
        udp_pcb = NULL;
    }

    udp_pcb = udp_new();

    if ( NULL == udp_pcb){
        return ERR_MEM;
    }

    err_t err = udp_bind(udp_pcb, IP_ADDR_ANY, listen_port);

    if(err != ERR_OK) {
        udp_remove(udp_pcb);
        udp_pcb = NULL;
        return err;
    }

    udp_recv(udp_pcb, udp_rx_callback, NULL);
    return ERR_OK;

}

void udp_source_deinit(void){

    SYS_ARCH_DECL_PROTECT(lev);

    if(NULL != udp_pcb){
        udp_remove(udp_pcb);
        udp_pcb = NULL;
    }
    SYS_ARCH_PROTECT(lev);
    s_rx_msg.valid = 0;
    SYS_ARCH_UNPROTECT(lev);
}

err_t udp_data_send(ip_addr_t *ip_addr_tx, ip_addr_t *ip_addr_rx,
                    u16_t port_number, const u8_t *data, u16_t data_len)
{

    /* TODO: GECICI COZUM — BYTE_ORDER derleme ayari (little-endian) ile
     * kartin gercek calisma modu (big-endian) arasindaki uyumsuzluk yuzunden
     * ip_addr_tx ve n->ip_addr byte-ters. Kok neden lwiplib.c'deki
     * lwIPNetifAdd/lwIPAliasAdd + BYTE_ORDER config'inde; duzeltilince bu
     * swap kaldirilmali. */

    if (NULL == udp_pcb || NULL == data || 0 == data_len)
    {
        return ERR_VAL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);

    if (NULL == p)
    {
        return ERR_MEM;
    }

    //compare ip_addr_tx with netif_list->ip_addr to find the correct netif
    //ip_addr_tx swapped because of little endian order (n->ip_addr has big endian order)

    ip_addr_t swapped_tx;
    swapped_tx.addr = byte_swap32(ip_addr_tx->addr);

    struct netif *n;
    for (n = netif_list; n != NULL; n = n->next) {
        if (ip_addr_cmp(&n->ip_addr, &swapped_tx)) {
            break;
        }
    }
    if( NULL == n ){ pbuf_free(p); return ERR_VAL; }

    memcpy(p->payload, data, data_len);

    ip_addr_t swapped_rx;
    swapped_rx.addr = byte_swap32(ip_addr_rx->addr);
    err_t err = udp_sendto_if(udp_pcb, p, &swapped_rx, port_number, n);

    pbuf_free(p);
    return err;

}


u8_t udp_source_poll_rx(udp_rx_msg_t *msg){

    //1 if reading succesfull, 0 if there is no data to read or unsuccesfull
    u8_t read_stat = 0;

    SYS_ARCH_DECL_PROTECT(lev);

    if(NULL == msg){
        return 0;
    }

    SYS_ARCH_PROTECT(lev);

    if(s_rx_msg.valid == 0){
        read_stat = 0;
    }

    //valid==1,  there is data to read
    else{
        *msg = *(udp_rx_msg_t *)&s_rx_msg;
        s_rx_msg.valid = 0;
        read_stat = 1;
        s_rx_count++;
    }
    SYS_ARCH_UNPROTECT(lev);
    return read_stat;
}


static void udp_rx_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            ip_addr_t *addr, u16_t port)
{
    if(NULL == p) return;

    if (1 == s_rx_msg.valid)
    {
        s_rx_drop_count++;
    }
    else
    {
        ip_addr_set(&s_rx_msg.src_ip, addr);

        s_rx_msg.src_port = port;

        u16_t copy_len = p->tot_len;
        if (copy_len > UDP_RX_BUF_SIZE)
        {
            copy_len = UDP_RX_BUF_SIZE;
        }
        s_rx_msg.data_len = copy_len;
        pbuf_copy_partial(p, (void *)s_rx_msg.data, s_rx_msg.data_len, 0);
        s_rx_msg.valid = 1;
    }

    pbuf_free(p);
}
















