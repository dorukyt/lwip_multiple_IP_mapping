/*
 * terminal_interface.c
 *
 *  Created on: 22 Tem 2026
 *      Author: PC_4434
 */

/*
 * Captures and displays the input of the user on the serial terminal
 * TODO:Changing the IP, GW and dest IP
 */
#include <stdio.h>

#include "netif/etharp.h"
#include "lwip/netif.h"
#include "ipv4/lwip/ip_addr.h"
#include "terminal_interface.h"

#define sciREGx sciREG1
uint8_t cmd_buf[CMD_BUFFER_SIZE];

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
    sciSend(sciREGx, sizeof(MsgMain) - 1, (uint8_t*) MsgMain);
    sciSend(sciREGx, sizeof(Msg_1) - 1, (uint8_t*) Msg_1);
    sciSend(sciREGx, sizeof(Msg_2) - 1, (uint8_t*) Msg_2);
    sciSend(sciREGx, sizeof(Msg_3) - 1, (uint8_t*) Msg_3);
    sciSend(sciREGx, sizeof(Msg_4) - 1, (uint8_t*) Msg_4);
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
        ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Change %c.IP address(%s) to:", ch, ip_str);
        sciSend(sciREGx, (uint32_t)len, (uint8_t *)line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if(ipaddr_aton((const char *)cmd_buf, &new_ip)){
         netif_set_ipaddr(n, &new_ip);
        }
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
        //TODO: Change this block of code to dest Ip address configurator
        // code block
        const char Msg[] = "Please chose which dest IP address you want to change:";
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
        ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
        len = snprintf(line, sizeof(line), "Change %c.GW address(%s) to:", ch, ip_str);
        sciSend(sciREGx, (uint32_t)len, (uint8_t *)line);
        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        if(ipaddr_aton((const char *)cmd_buf, &new_gw)){
         netif_set_gw(n, &new_gw);
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
