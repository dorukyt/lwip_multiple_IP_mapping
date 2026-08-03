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

static udp_listener_t s_listeners[UDP_MAX_LISTENERS];

//assigns a unique ID to the UDP socket
//idx is the line number of that id
//generation is dependent on how many iterations has there been(its used as a verification mechanism)
static udp_sock_id_t make_id(u8_t idx)
{
    u16_t generation, socket_id;
    generation = (u16_t) s_listeners[idx].generation_count;
    socket_id = generation << 8 | idx;
    return socket_id;
}

//disassembles the id to gen and idx
//validates if the generation matches current one and the socket is in use
//@arg id: takes the unique ID of a socket
//@return idx: returns the line number of the socket in s_listeners list
static s8_t resolve(udp_sock_id_t id)
{
    u8_t idx, generation;
    idx = (u8_t) (id & 0xFF);
    generation = (u8_t) (id >> 8);

    if (idx >= UDP_MAX_LISTENERS || 0 == s_listeners[idx].in_use
            || generation != s_listeners[idx].generation_count)
    {
        return -1;
    }
    return idx;
}

static void remove_slot(u8_t idx)
{
    if (NULL != s_listeners[idx].pcb)
    {
        udp_remove(s_listeners[idx].pcb);
        s_listeners[idx].pcb = NULL;
    }
    s_listeners[idx].rx_msg.valid = 0;
    s_listeners[idx].in_use = 0;
    s_listeners[idx].generation_count++;
}

static void udp_rx_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                            ip_addr_t *addr, u16_t port);

//call after a netif's ip_addr has changed; rebinds every listener on that netif
void udp_source_netif_ip_changed(struct netif *netif)
{
    u8_t idx;
    SYS_ARCH_DECL_PROTECT(lev);

    if (netif == NULL)
        return;

    SYS_ARCH_PROTECT(lev);
    for (idx = 0; idx < UDP_MAX_LISTENERS; idx++)
    {
        if (s_listeners[idx].in_use && s_listeners[idx].netif == netif)
        {
            udp_bind(s_listeners[idx].pcb, &netif->ip_addr,
                     s_listeners[idx].pcb->local_port);
        }
    }
    SYS_ARCH_UNPROTECT(lev);
}

//adds a listening port for the designated IP and Port number
//@arg local_ip: chose the netif with the IP user want to use to send data
//@arg local_port: chose which port number to send the data from
//@arg socket_id: returns the distinct id number for the s_listeners[idx] list
//@return err_t: returns the error code of the operation
err_t udp_source_add_listener(struct netif *netif, u16_t local_port,
                              udp_sock_id_t *socket_id)
{

    u8_t idx;
    err_t err;
    struct udp_pcb *new_pcb;

    //check if users input has null pointers
    if (netif == NULL || socket_id == NULL)
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
            err = udp_bind(new_pcb, &netif->ip_addr, local_port);
            if (err != ERR_OK)
            {
                udp_remove(new_pcb);
                new_pcb = NULL;
                return err;
            }

            //place the new pcb to receive the data
            udp_recv(new_pcb, udp_rx_callback, (void*) (uintptr_t) idx);
            s_listeners[idx].pcb = new_pcb;
            s_listeners[idx].netif = netif;
            s_listeners[idx].in_use = 1;
            s_listeners[idx].rx_count = 0;
            s_listeners[idx].rx_drop_count = 0;
            s_listeners[idx].rx_msg.valid = 0;

            *socket_id = make_id(idx);

            return ERR_OK;

        }
    }
    return ERR_MEM;
}

void udp_source_remove_listener(udp_sock_id_t socket_id)
{
    SYS_ARCH_DECL_PROTECT(lev);
    s8_t idx;
    idx = resolve(socket_id);
    if (idx < 0)
        return;

    SYS_ARCH_PROTECT(lev);
    remove_slot(idx);
    SYS_ARCH_UNPROTECT(lev);
}

void udp_source_remove_all_on_netif(struct netif *netif)
{
    SYS_ARCH_DECL_PROTECT(lev);
    u8_t idx;

    if (NULL == netif)
        return;
    SYS_ARCH_PROTECT(lev);
    for (idx = 0; idx < UDP_MAX_LISTENERS; idx++)
    {
        if (netif == s_listeners[idx].netif && s_listeners[idx].in_use == 1)
        {
            remove_slot(idx);
        }
    }
    SYS_ARCH_UNPROTECT(lev);
}

//sends the prepared data to the designated IP address and Port
//also expects the sender IP to comply with IP aliasing
err_t udp_source_data_send(udp_sock_id_t socket_id, struct netif *tx_netif,
                    ip_addr_t *ip_addr_rx, u16_t port_number, const u8_t *data,
                    u16_t data_len)
{
    s8_t idx;
    idx = resolve(socket_id); //idx = -1 if socket_id isn't correct
//return ERR if there is no tx structure or data on the line
//amount of udp ports exceeds the udp_max_listeners or the port isn't in use
    if (NULL == tx_netif || NULL == data || 0 == data_len || idx < 0)
    {
        return ERR_VAL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_RAM);

    if (NULL == p)
    {
        return ERR_MEM;
    }

    memcpy(p->payload, data, data_len);

    err_t err = udp_sendto_if(s_listeners[idx].pcb, p, ip_addr_rx, port_number,
                              tx_netif);

    pbuf_free(p);
    return err;

}

//main loop has to call this function to fetch the data that is stored in the message box
//that had a data from the EMAC driver
u8_t udp_source_poll_rx(udp_sock_id_t socket_id, udp_rx_msg_t *msg)
{

    //1 if reading succesfull, 0 if there is no data to read or unsuccesfull
    u8_t read_stat = 0;

    SYS_ARCH_DECL_PROTECT(lev);
    s8_t idx;
    idx = resolve(socket_id); //idx = -1 if socket_id isn't correct
    //check if there is a message and we have a pcb to store it
    if (NULL == msg || idx < 0)
    {
        return 0;
    }

    //stop interrupts to to prevent reading and writing at the same time to the message box
    SYS_ARCH_PROTECT(lev);

    if (s_listeners[idx].rx_msg.valid == 0)
    {
        read_stat = 0;
    }

    //valid==1,  there is data to read
    else
    {
        *msg = *(udp_rx_msg_t*) &s_listeners[idx].rx_msg;
        s_listeners[idx].rx_msg.valid = 0;
        read_stat = 1;
        s_listeners[idx].rx_count++;
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
    u8_t idx = (u8_t) (uintptr_t) arg;
    if (!(idx < UDP_MAX_LISTENERS && s_listeners[idx].in_use == 1))
    {
        return;
    }

    if (NULL == p)
        return;

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
        pbuf_copy_partial(p, (void*) s_listeners[idx].rx_msg.data,
                          s_listeners[idx].rx_msg.data_len, 0);
        s_listeners[idx].rx_msg.valid = 1;
    }

    pbuf_free(p);
}

u16_t udp_source_get_local_port(udp_sock_id_t socket_id)
{
    s8_t idx;
    idx = resolve(socket_id); //idx = -1 if socket_id isn't correct
    if (idx < 0)
        return 0;

    return s_listeners[idx].pcb->local_port;
}

u8_t udp_source_count_sockets(struct netif *netif)
{
    u8_t socket_count = 0;
    u8_t idx;

    if(NULL == netif)
        return 0;

    for(idx = 0; idx < UDP_MAX_LISTENERS; idx++)
    {
        if(netif == s_listeners[idx].netif && 1 == s_listeners[idx].in_use)
            socket_count++;
    }
    return socket_count;
}

udp_netif_socket_info_t udp_source_get_socket(struct netif *netif, u8_t nth)
{
    u8_t socket_count = 0;
    u8_t idx;
    udp_netif_socket_info_t udp_socket;
    udp_socket.local_port = 0;
    udp_socket.netif = NULL;
    udp_socket.socket_id = UDP_SOCK_ID_INVALID;

    if (NULL == netif)
    {
        return udp_socket;
    }

    for (idx = 0; idx < UDP_MAX_LISTENERS; idx++)
    {

        if (netif == s_listeners[idx].netif && 1 == s_listeners[idx].in_use)
        {
            if (nth == socket_count)
            {
                udp_socket.netif = netif;
                udp_socket.local_port = s_listeners[idx].pcb->local_port;
                udp_socket.socket_id = make_id(idx);
                break;
            }
            socket_count++;
        }
    }

    return udp_socket;
}
