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


err_t udp_source_init(u16_t listen_port){

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

    if (NULL == udp_pcb)
    {
        return ERR_VAL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);

    if (NULL == p)
    {
        return ERR_MEM;
    }

    memcpy(p->payload, data, data_len);

    ip_addr_set(&udp_pcb->local_ip, ip_addr_tx);

    err_t err = udp_sendto(udp_pcb, p,ip_addr_rx, port_number);

    ip_addr_set_any(&udp_pcb->local_ip);

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
















