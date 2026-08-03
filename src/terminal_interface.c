/*
 * terminal_interface.c
 *
 *  Created on: 22 Tem 2026
 *      Author: PC_4434
 */

/*
 * Captures and displays the input of the user on the serial terminal
 */
#include <stdio.h>
#include <stdlib.h>

#include "netif/etharp.h"
#include "lwip/netif.h"
#include "ipv4/lwip/ip_addr.h"
#include "terminal_interface.h"
#include "UDP_source.h"
#include "ping.h"
#include "net_manager.h"
#include "string.h"

#define sciREGx sciREG1
uint8_t cmd_buf[CMD_BUFFER_SIZE];

static dest_node_t dest_pool[MAX_DEST];

char Spacer[] = "*--------------------------------------*\r\n\n";

dest_node_t *dest_list_add(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    dest_node_t *new_node = NULL;

    int i;
    for(i = 0; i < MAX_DEST; i++)
    {
        if(dest_pool[i].in_use == 0)
        {
        new_node = &dest_pool[i];
        break;
        }
    }
    if(NULL == new_node) return NULL;
    new_node->in_use = 1;
    IP4_ADDR(&new_node->dest_addr, a , b , c , d);

    new_node->next = dest_list_head;
    dest_list_head = new_node;

    return new_node;
}

void read_terminal_line(void)
{
    uint32_t idx = 0;
    uint8_t ch;

    while (sciIsRxReady(sciREGx))
    {
        (void) sciReceiveByte(sciREGx);
    }

    /*
     * Initial interface for the user
     * 1.Change IP address
     * 2.Change GW address
     * 3.Change the dest IP
     * 4.Reset the ARP tables(resets all the ARP tables)
     */
    const char MsgMain[] = "\r\nSelect the option you want to configure:\r\n";
    const char Msg_1[] = "1. Change IP address\r\n";
    const char Msg_2[] = "2. Change GW address\r\n";
    const char Msg_3[] = "3. Change the dest IP\r\n";
    const char Msg_4[] = "4. Reset the ARP tables\r\n";
    const char Msg_5[] = "5. Ping an IP address\r\n";
    const char Msg_6[] = "6. Add network interface\r\n";
    const char Msg_7[] = "7. Delete network interface\r\n";
    const char Msg_8[] = "8. Open Socket\r\n";
    const char Msg_9[] = "9. Delete Socket\r\n";
    sciSend(sciREGx, sizeof(MsgMain) - 1, (uint8_t*) MsgMain);
    sciSend(sciREGx, sizeof(Msg_1) - 1, (uint8_t*) Msg_1);
    sciSend(sciREGx, sizeof(Msg_2) - 1, (uint8_t*) Msg_2);
    sciSend(sciREGx, sizeof(Msg_3) - 1, (uint8_t*) Msg_3);
    sciSend(sciREGx, sizeof(Msg_4) - 1, (uint8_t*) Msg_4);
    sciSend(sciREGx, sizeof(Msg_5) - 1, (uint8_t*) Msg_5);
    sciSend(sciREGx, sizeof(Msg_6) - 1, (uint8_t*) Msg_6);
    sciSend(sciREGx, sizeof(Msg_7) - 1, (uint8_t*) Msg_7);
    sciSend(sciREGx, sizeof(Msg_8) - 1, (uint8_t*) Msg_8);
    sciSend(sciREGx, sizeof(Msg_9) - 1, (uint8_t*) Msg_9);
    sciSendByte(sciREGx, '\r');
    sciSendByte(sciREGx, '\n');

    sciReceive(sciREGx, 1, &ch);
    sciSendByte(sciREGx, ch);
    sciSendByte(sciREGx, '\r');
    sciSendByte(sciREGx, '\n');

    switch (ch)
    {

    case '1':
    {
        // code block
        const char Msg[] = "Please chose which IP address you want to change:";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        uint8_t i = 1;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[48];
        ip_addr_t new_ip;

        for (n = netif_list; n != NULL; n = n->next)
        {
            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "%d.IP adresi: %s\r\n", i, ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            i++;
        }

        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while(n != NULL && idx-- > 0){
            n = n->next;
        }
        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Change %c.IP address(%s) to:", ch, ip_str);
        sciSend(sciREGx, (uint32_t)len, (uint8_t *)line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);

        if(ipaddr_aton((const char *)cmd_buf, &new_ip)){
         netif_set_ipaddr(n, &new_ip);
        }
        udp_source_netif_ip_changed(n);
        break;
    }

    case '2':
    {
        // code block
        const char Msg[] = "Please chose which GW address you want to change:";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        uint8_t i = 1;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[48];
        ip_addr_t new_gw;

        for (n = netif_list; n != NULL; n = n->next)
        {
            ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
            len = sprintf(line, "%d.GW adress: %s\r\n", i, ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            i++;
        }

        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while(n != NULL && idx-- > 0){
            n = n->next;
        }

        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }
        ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Change %c.GW address(%s) to:", ch, ip_str);
        sciSend(sciREGx, (uint32_t)len, (uint8_t *)line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if(ipaddr_aton((const char *)cmd_buf, &new_gw)){
         netif_set_gw(n, &new_gw);
        }
        break;
    }

    case '3':
    {
        // code block
        const char Msg[] = "Please chose which dest IP address you want to change:";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        uint8_t i = 1;
        int len;
        dest_node_t *n;
        char ip_str[16];
        char line[48];
        ip_addr_t new_dest;

        for (n = dest_list_head; n != NULL; n = n->next)
        {
            ipaddr_ntoa_r(&n->dest_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "%d.dest adress: %s\r\n", i, ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            i++;
        }

        n = dest_list_head;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while(n != NULL && idx-- > 0){
            n = n->next;
        }

        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        ipaddr_ntoa_r(&n->dest_addr, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Change %c.dest address(%s) to:", ch, ip_str);
        sciSend(sciREGx, (uint32_t)len, (uint8_t *)line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        //give new_dest the address from cmd_buf
        if(ipaddr_aton((const char *)cmd_buf, &new_dest)){
         n->dest_addr = new_dest;
        }
        break;
    }

    case '4':
    {
        // code block
        struct netif *n;
        for (n = netif_list; n != NULL; n = n->next)
        {
            etharp_cleanup_netif(n);
        }
        const char Msg[] = "ARP Table reset completed";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');
    }
        break;

    case '5':
    {
        const char Msg[] = "Please chose which netif you want to ping from:";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        uint8_t i = 1;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];
        ip_addr_t target;

        for (n = netif_list; n != NULL; n = n->next)
        {
            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "%d.netif IP: %s\r\n", i, ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            i++;
        }

        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while (n != NULL && idx-- > 0)
        {
            n = n->next;
        }

        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Ping from %s to:", ip_str);
        sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);

        if (!ipaddr_aton((const char*) cmd_buf, &target))
        {
            const char Err[] = "Invalid IP... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        {
            char tgt_str[16];
            int attempt;

            ipaddr_ntoa_r(&target, tgt_str, sizeof(tgt_str));
            len = snprintf(line, sizeof(line), "Pinging %s from %s:\r\n",
                           tgt_str, ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            for (attempt = 0; attempt < PING_ATTEMPT_COUNT; attempt++)
            {
                ping_result_t res;
                err_t perr = ping_send(n, &target);

                if (perr != ERR_OK)
                {
                    len = snprintf(line, sizeof(line),
                                   "Ping send err: %d\r\n", (int) perr);
                    sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
                    break;
                }

                if (ping_wait_reply(PING_TIMEOUT_MS, &res))
                {
                    char from_str[16];
                    ipaddr_ntoa_r(&res.from, from_str, sizeof(from_str));

                    if (0 == res.rtt_ms)
                    {
                        len = snprintf(line, sizeof(line),
                                "Reply from %s: seq=%u bytes=%u time<1ms TTL=%u\r\n",
                                from_str, (unsigned) res.seqno,
                                (unsigned) res.data_len, (unsigned) res.ttl);
                    }
                    else
                    {
                        len = snprintf(line, sizeof(line),
                                "Reply from %s: seq=%u bytes=%u time=%lums TTL=%u\r\n",
                                from_str, (unsigned) res.seqno,
                                (unsigned) res.data_len,
                                (unsigned long) res.rtt_ms, (unsigned) res.ttl);
                    }
                    sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
                }
                else
                {
                    const char Tmo[] = "Request timed out\r\n";
                    sciSend(sciREGx, sizeof(Tmo) - 1, (uint8_t*) Tmo);
                }
            }
        }
        break;
    }

    case '6':
    {
        // code block
        char Msg[] = "Please write the interface you want to add\r\n";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

        ip_addr_t new_ip;
        ip_addr_t new_netmask;
        ip_addr_t new_gw;

        const char PromptIP[] = "IP Address: ";
        sciSend(sciREGx, sizeof(PromptIP) - 1, (uint8_t*) PromptIP);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if (!(ipaddr_aton((const char *)cmd_buf, &new_ip)))
        {
            const char Msg[] = "Invalid value, netif not saved\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            break;
        }

        const char PromptNetMask[] = "Netmask Address: ";
        sciSend(sciREGx, sizeof(PromptNetMask) - 1, (uint8_t*) PromptNetMask);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if (!(ipaddr_aton((const char *)cmd_buf, &new_netmask)))
        {
            const char Msg[] = "Invalid value, netif not saved\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            break;
        }

        const char PromptGW[] = "GW Address: ";
        sciSend(sciREGx, sizeof(PromptGW) - 1, (uint8_t*) PromptGW);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if (!(ipaddr_aton((const char *)cmd_buf, &new_gw)))
        {
            const char Msg[] = "Invalid value, netif not saved\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            break;
        }

        if(ERR_OK == net_if_add(&new_ip, &new_netmask, &new_gw))
        {
            const char Msg[] = "\r\nNew netif has been succesfully created\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        }
        break;
    }

    case '7':
    {
        // code block

        uint8_t i = 1;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];

        char Msg[] = "Please chose the interface you want to delete\r\n\n";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

        //writes the netif info(IP,Netmask,GW) for all the netifs
        for (n = netif_list; n != NULL; n = n->next)
        {
            len = sprintf(line, "%d.netif\r\n", i);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "IP:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->netmask, ip_str, sizeof(ip_str));
            len = sprintf(line, "Netmask: %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
            len = sprintf(line, "GW:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            sciSendByte(sciREGx, '\n');
            i++;
        }

        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while(n != NULL && idx-- > 0){
            n = n->next;
        }
        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }
        if(ERR_OK == net_if_remove(n))
        {
            const char Msg[] = "\r\nNetif has been succesfully deleted\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            break;
        }

        const char Err[] = "There has been an error while deleting the netif\r\n";
        sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
        break;
    }

    case '8':
    {
        // code block


        uint8_t i = 1;
        uint8_t j;
        u16_t socket_port;
        udp_sock_id_t new_id;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];

        char Msg0[] = "Please chose the interface you want to add socket to\r\n\n";
        sciSend(sciREGx, sizeof(Msg0) - 1, (uint8_t*) Msg0);
        sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);

        //writes the netif info(IP,Netmask,GW) for all the netifs
        for (n = netif_list; n != NULL; n = n->next)
        {
            len = sprintf(line, "%d.netif\r\n", i);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "IP:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->netmask, ip_str, sizeof(ip_str));
            len = sprintf(line, "Netmask: %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
            len = sprintf(line, "GW:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            sciSendByte(sciREGx, '\n');

            char Msg[] ="Active Sockets:\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

            if (!(0 == udp_source_count_sockets(n)))
            {
                for (j = 1; j < UDP_MAX_LISTENERS + 1; j++)
                {
                    if (UDP_SOCK_ID_INVALID
                            != udp_source_get_socket(n, j - 1).socket_id)
                    {
                        socket_port =
                                udp_source_get_socket(n, j - 1).local_port;
                        len = sprintf(line, "%d.socket(local_port): %u\r\n", j,
                                      socket_port);
                        sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
                    }

                }
            }
            else
            {
                char Err[] = "No socket is present\r\n\n";
                sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            }
            i++;
            sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);
        }

//TODO: There is a bug here, I can't appoint more then 2 ports

        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t)(ch - '0');
        idx--;
        while(n != NULL && idx-- > 0){
            n = n->next;
        }
        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        char Msg1[] ="Input new socket port:\r\n";
        sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);

        fetch_input(cmd_buf, CMD_BUFFER_SIZE);            // tüm satýrý cmd_buf'a oku
        socket_port = (u16_t) atoi((const char*) cmd_buf);       // string -> sayý

        if(ERR_OK == udp_source_add_listener(n, socket_port , &new_id))
        {
            const char Msg[] = "\r\nSocket has been succesfully added\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            break;
        }

        const char Err[] = "There has been an error while adding the socket\r\n";
        sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
        break;
    }


    //TODO: It didn't delete the socket
    case '9':
    {
        // code block

        uint8_t i = 1;
        uint8_t j;
        u8_t count = 0;
        u16_t socket_port;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];

        char Msg0[] =
                "Please chose the interface you want to delete a socket from\r\n\n";
        sciSend(sciREGx, sizeof(Msg0) - 1, (uint8_t*) Msg0);
        sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);

        //writes the netif info(IP,Netmask,GW) for all the netifs
        for (n = netif_list; n != NULL; n = n->next)
        {
            len = sprintf(line, "%d.netif\r\n", i);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = sprintf(line, "IP:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->netmask, ip_str, sizeof(ip_str));
            len = sprintf(line, "Netmask: %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

            ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
            len = sprintf(line, "GW:      %s\r\n", ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            sciSendByte(sciREGx, '\r');
            sciSendByte(sciREGx, '\n');

            //write the active sockets in a list to terminal
            char Msg[] = "Active Sockets:\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

            if (!(0 == udp_source_count_sockets(n)))
            {
                for (j = 0; j < udp_source_count_sockets(n); j++)
                {
                    udp_netif_socket_info_t info = udp_source_get_socket(n, j);
                    socket_port = udp_source_get_socket(n, j).local_port;
                    len = sprintf(line, "%d.socket(local_port): %u\r\n", j + 1, socket_port);
                    sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
                    count++;
                }
            }
            else
            {
                char Err[] = "No socket is present\r\n\n";
                sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            }

            i++;
            sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);
        }


        n = netif_list;
        sciReceive(sciREGx, 1, &ch);
        idx = (uint32_t) (ch - '0');
        idx--;
        while (n != NULL && idx-- > 0)
        {
            n = n->next;
        }
        if (n == NULL)
        {
            const char Err[] = "Invalid selection... Exiting\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

        count = 0;
        char Msg1[] = "Select which socket you want to delete:\r\n";
        sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);

        for (j = 0; j < udp_source_count_sockets(n); j++)
        {
            socket_port = udp_source_get_socket(n, j).local_port;
            len = sprintf(line, "%d.socket(local_port): %u\r\n", j + 1, socket_port);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }

        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        uint32_t sel = (uint32_t) atoi((const char*) cmd_buf);   // 1 tabanlý seçim

        if(sel < 1 || sel > udp_source_count_sockets(n))
            {
            char Msg1[] = "Invalid Socket. Procedure not completed\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
            break;
            }

        udp_sock_id_t chosen = udp_source_get_socket(n, sel - 1).socket_id;  // numara -> id
        udp_source_remove_listener(chosen);            // id DEÐERLE geçilir (& YOK)

        const char Msg[] = "\r\nSocket has been succesfully deleted\r\n";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
    }

    default:
    {
        const char Msg[] = "Invalid Key... Exiting";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');
        // code block
    }
    }
    return;
}

void fetch_input(uint8_t *buf, uint32_t max_len)
{
    uint32_t idx = 0;
    uint8_t ch;

    while (sciIsRxReady(sciREGx))
    {
        (void) sciReceiveByte(sciREGx);
    }

    while (idx < max_len - 1)
    {
        sciReceive(sciREGx, 1, &ch);

        //break if enter pressed
        if (ch == '\r' || ch == '\n')
        {
            sciSendByte(sciREGx, '\r');
            sciSendByte(sciREGx, '\n');
            break;
        }

        //delete from buffer if backspace
        //do nothing if the buffer is empty
        else if (ch == '\x7f' || ch == '\x08')
        {
            if (idx > 0)
            {
                sciSendByte(sciREGx, '\b');
                sciSendByte(sciREGx, ' ');
                sciSendByte(sciREGx, '\b');
                buf[--idx] = '\0';
            }
        }
        else
        {
            sciSendByte(sciREGx, ch);
            buf[idx++] = ch;
        }
    }
    buf[idx] = '\0';
    return;
}
