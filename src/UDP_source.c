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
#include <stdint.h>

static volatile udp_rx_msg_t s_rx_msg;

static udp_listener_t s_listeners[UDP_MAX_LISTENERS];

static void udp_rx_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            ip_addr_t *addr, u16_t port);

//adds a listening port for the designated IP and Port number
err_t udp_source_add_listener(ip_addr_t *local_ip, u16_t local_port,
                              u8_t *handle_out)
{

    u8_t idx;
    err_t err;
    struct udp_pcb *new_pcb;

    //check if users input has null pointers
    if (local_ip == NULL || handle_out == NULL)
    {
        return ERR_VAL;
    }

    //scan for empty pcb slots, return err if there is none
    for (idx = 0; idx < UDP_MAX_LISTENERS; idx++)
    {
        if (s_listeners[idx].in_use == 0)
        {

            new_pcb = udp_new();
            if (new_pcb == NULL)
            {
                return ERR_MEM;
            }

            //create a new pcb in empty spot
            err = udp_bind(new_pcb, local_ip, local_port);
            if (err != ERR_OK)
            {
                udp_remove(new_pcb);
                new_pcb = NULL;
                return err;
            }

            //place the new pcb to receive the data
            udp_recv(new_pcb, udp_rx_callback, (void*) (uintptr_t) idx);
            s_listeners[idx].pcb = new_pcb;
            s_listeners[idx].in_use = 1;
            s_listeners[idx].rx_count = 0;
            s_listeners[idx].rx_drop_count = 0;
            s_listeners[idx].rx_msg.valid = 0;
            *handle_out = idx;

            return ERR_OK;

        }
    }
    return ERR_MEM;
}


void udp_source_remove_listener(u8_t handle)
{
    SYS_ARCH_DECL_PROTECT(lev);
    if (!(handle < UDP_MAX_LISTENERS && s_listeners[handle].in_use == 1))
    {
        return;
    }

    SYS_ARCH_PROTECT(lev);
    if (NULL != s_listeners[handle].pcb)
    {
        udp_remove(s_listeners[handle].pcb);
        s_listeners[handle].pcb = NULL;
        s_listeners[handle].in_use = 0;
    }
    s_listeners[handle].rx_msg.valid = 0;
    SYS_ARCH_UNPROTECT(lev);
}


//sends the prepared data to the designated IP address and Port
//also expects the sender IP to comply with IP aliasing
err_t udp_data_send(u8_t handle, struct netif *tx_netif, ip_addr_t *ip_addr_rx,
                    u16_t port_number, const u8_t *data, u16_t data_len)
{

    if (NULL == tx_netif || NULL == data || 0 == data_len
            || !(handle < UDP_MAX_LISTENERS && s_listeners[handle].in_use == 1))
    {
        return ERR_VAL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);

    if (NULL == p)
    {
        return ERR_MEM;
    }

    memcpy(p->payload, data, data_len);

    err_t err = udp_sendto_if(s_listeners[handle].pcb, p, ip_addr_rx, port_number, tx_netif);

    pbuf_free(p);
    return err;

}

//main loop has to call this function to fetch the data that is stored in the message box
//that had a data from the EMAC driver
u8_t udp_source_poll_rx(u8_t handle, udp_rx_msg_t *msg){

    //1 if reading succesfull, 0 if there is no data to read or unsuccesfull
    u8_t read_stat = 0;

    SYS_ARCH_DECL_PROTECT(lev);

    //check if there is a message and we have a pcb to store it
    if(NULL == msg || !(handle < UDP_MAX_LISTENERS && s_listeners[handle].in_use == 1)){
        return 0;
    }

    //stop interrupts to to prevent reading and writing at the same time to the message box
    SYS_ARCH_PROTECT(lev);

    if(s_listeners[handle].rx_msg.valid == 0){
        read_stat = 0;
    }

    //valid==1,  there is data to read
    else{
        *msg = *(udp_rx_msg_t *)&s_listeners[handle].rx_msg;
        s_listeners[handle].rx_msg.valid = 0;
        read_stat = 1;
        s_listeners[handle].rx_count++;
    }
    SYS_ARCH_UNPROTECT(lev);
    return read_stat;
}

//copies the data EMAC driver gives into a udp_pcb
//we have to pull this data later to process it otherwise the data box stays full
//if the data is not emptied, consecutive packets gets dropped
static void udp_rx_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            ip_addr_t *addr, u16_t port)
{
    u8_t idx = (u8_t)(uintptr_t)arg;
    if( !(idx < UDP_MAX_LISTENERS && s_listeners[idx].in_use == 1)){
        return;
    }


    if(NULL == p) return;

    if (1 == s_listeners[idx].rx_msg.valid)
    {
        s_listeners[idx].rx_drop_count++;
    }
    else
    {
        ip_addr_set(&s_listeners[idx].rx_msg.src_ip, addr);

        s_listeners[idx].rx_msg.src_port = port;

        u16_t copy_len = p->tot_len;
        if (copy_len > UDP_RX_BUF_SIZE)
        {
            copy_len = UDP_RX_BUF_SIZE;
        }
        s_listeners[idx].rx_msg.data_len = copy_len;
        pbuf_copy_partial(p, (void *)s_listeners[idx].rx_msg.data, s_listeners[idx].rx_msg.data_len, 0);
        s_listeners[idx].rx_msg.valid = 1;
    }

    pbuf_free(p);
}

u16_t udp_source_get_local_port(u8_t handle)
{
    if (!(handle < UDP_MAX_LISTENERS && s_listeners[handle].in_use == 1))
    {
        return 0;
    }
    return s_listeners[handle].pcb->local_port;
}
