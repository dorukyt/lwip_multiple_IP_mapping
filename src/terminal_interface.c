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
#include "HL_rti.h"

#define sciREGx sciREG1
#define TERM_CTRLC 0x03
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

/* Tek karakterlik secim okumadan once hatta bekleyen baytlari
 * (ornegin onceki satirdan kalan '\r') temizler, sonra 1 karakter okur.
 * Masaustu uygulamasi her gonderime '\r' ekledigi icin gereklidir. */
static uint8_t read_choice_char(void)
{
    uint8_t ch;
    while (sciIsRxReady(sciREGx)) { (void) sciReceiveByte(sciREGx); }
    sciReceive(sciREGx, 1, &ch);
    return ch;
}

void read_terminal_line(void)
{
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
    const char Msg_1[] = "1. List Netif's\r\n";
    const char Msg_2[] = "2. Configure Netif\r\n";
    const char Msg_3[] = "3. Add/Delete Netif\r\n";
    const char Msg_4[] = "4. UDP Data Send\r\n";
    const char Msg_5[] = "5. Ping an IP address\r\n";
    const char Msg_6[] = "6. Reset the ARP tables\r\n";
    const char Msg_7[] = "7. Scan Network\r\n";

    sciSend(sciREGx, sizeof(MsgMain) - 1, (uint8_t*) MsgMain);
    sciSend(sciREGx, sizeof(Msg_1) - 1, (uint8_t*) Msg_1);
    sciSend(sciREGx, sizeof(Msg_2) - 1, (uint8_t*) Msg_2);
    sciSend(sciREGx, sizeof(Msg_3) - 1, (uint8_t*) Msg_3);
    sciSend(sciREGx, sizeof(Msg_4) - 1, (uint8_t*) Msg_4);
    sciSend(sciREGx, sizeof(Msg_5) - 1, (uint8_t*) Msg_5);
    sciSend(sciREGx, sizeof(Msg_6) - 1, (uint8_t*) Msg_6);
    sciSend(sciREGx, sizeof(Msg_7) - 1, (uint8_t*) Msg_7);
    sciSendByte(sciREGx, '\r');
    sciSendByte(sciREGx, '\n');

    fetch_input(cmd_buf, CMD_BUFFER_SIZE);          // tüm satýrý oku (echo + Enter zaten fetch_input'ta)

    /* --- komut ayrýþtýrma: "ping <IP>" --- */
    if (strncmp((const char*) cmd_buf, "ping ", 5) == 0)
    {
        terminal_ping_command((const char*) (cmd_buf + 5));   // "ping " sonrasý = IP
        return;
    }

    /* boþ satýr -> yoksay */
    if (cmd_buf[0] == '\0') return;

    /* rakamla baþlamýyorsa -> bilinmeyen komut */
    if (cmd_buf[0] < '0' || cmd_buf[0] > '9')
    {
        const char Err[] = "Unknown command\r\n";
        sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
        return;
    }

    int sel = atoi((const char*) cmd_buf);   /* TÜM satýr -> sayý (çok haneli) */
    switch (sel)
    {

    //lists all the active netifs we have
    case 1:
    {
        terminal_list_netif();
        break;
    }

    //Netif configuration interface
    //Lets the user change IP, Netmask and GW addresses
    //Add or delete  UDP sockets
    case 2:
    {
        ip_addr_t new_ip;
        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];


        char MsgNetifSelect[] ="Please chose the interface you want to edit\r\n\n";
        sciSend(sciREGx, sizeof(MsgNetifSelect) - 1, (uint8_t*) MsgNetifSelect);
        n = terminal_select_netif();
        if(NULL == n) break;

        char MsgOptions[] = "What do you want to edit?\r\n";
        sciSend(sciREGx, sizeof(MsgOptions) - 1, (uint8_t*) MsgOptions);
        char MsgIP[] = "1. IP Adress\r\n";
        sciSend(sciREGx, sizeof(MsgIP) - 1, (uint8_t*) MsgIP);
        char MsgNetMask[] = "2. NetMask\r\n";
        sciSend(sciREGx, sizeof(MsgNetMask) - 1, (uint8_t*) MsgNetMask);
        char MsgGW[] = "3. Default GateWay\r\n";
        sciSend(sciREGx, sizeof(MsgGW) - 1, (uint8_t*) MsgGW);
        char MsgOpenSocket[] = "4. Open New Socket\r\n";
        sciSend(sciREGx, sizeof(MsgOpenSocket) - 1, (uint8_t*) MsgOpenSocket);
        char MsgCloseSocket[] ="5. Delete Socket\r\n\n";
        sciSend(sciREGx, sizeof(MsgCloseSocket) - 1, (uint8_t*) MsgCloseSocket);

        ch = read_choice_char();
        sciSendByte(sciREGx, ch);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');


        switch (ch)
        {

        //change IP
        case '1':
        {
            ipaddr_ntoa_r(&n->ip_addr, ip_str, sizeof(ip_str));
            len = snprintf(line, sizeof(line), "Change IP address(%s) to:",ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            fetch_input(cmd_buf, CMD_BUFFER_SIZE);

            if (ipaddr_aton((const char*) cmd_buf, &new_ip))
            {
                netif_set_ipaddr(n, &new_ip);
            }
            udp_source_netif_ip_changed(n);
            break;
        }

            //Change NetMask
        case '2':
        {
            ip_addr_t new_netmask;
            ipaddr_ntoa_r(&n->netmask, ip_str, sizeof(ip_str));
            len = snprintf(line, sizeof(line), "Change NetMask(%s) to:",ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            fetch_input(cmd_buf, CMD_BUFFER_SIZE);
            if (ipaddr_aton((const char*) cmd_buf, &new_netmask))
            {
                netif_set_netmask(n, &new_netmask);
            }
            break;
        }
            //change GW
        case '3':
        {
            ip_addr_t new_gw;
            ipaddr_ntoa_r(&n->gw, ip_str, sizeof(ip_str));
            len = snprintf(line, sizeof(line), "Change GW address(%s) to:",ip_str);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            fetch_input(cmd_buf, CMD_BUFFER_SIZE);
            if (ipaddr_aton((const char*) cmd_buf, &new_gw))
            {
                netif_set_gw(n, &new_gw);
            }
            break;
        }

            //Adds a UDP socket with written port number to the chosen netif
        case '4':
        {
            // code block
            u16_t socket_port;
            udp_sock_id_t new_id;
            struct netif *n;

            char Msg0[] =
                    "Please chose the interface you want to add socket to\r\n\n";
            sciSend(sciREGx, sizeof(Msg0) - 1, (uint8_t*) Msg0);
            sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);

            //writes the netif info(IP,Netmask,GW) for all the netifs
            n = terminal_select_netif();
            if (NULL == n)
                break;

            char Msg1[] = "Input new socket port:\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);

            fetch_input(cmd_buf, CMD_BUFFER_SIZE);   // tüm satýrý cmd_buf'a oku
            socket_port = (u16_t) atoi((const char*) cmd_buf); // string -> sayý

            if (ERR_OK == udp_source_add_listener(n, socket_port, &new_id))
            {
                const char Msg[] = "\r\nSocket has been succesfully added\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
                break;
            }

            const char Err[] =
                    "There has been an error while adding the socket\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
        }

            //Deletes the selected UDP socket from a netif
        case '5':
        {
            // code block
            uint8_t j;
            u16_t socket_port;
            int len;
            struct netif *n;
            char line[96];

            char Msg0[] =
                    "Please chose the interface you want to delete a socket from\r\n\n";
            sciSend(sciREGx, sizeof(Msg0) - 1, (uint8_t*) Msg0);
            sciSend(sciREGx, sizeof(Spacer) - 1, (uint8_t*) Spacer);

            //writes the netif info(IP,Netmask,GW) for all the netifs
            n = terminal_select_netif();
            if (NULL == n)
                break;

            if (0 == udp_source_count_sockets(n))
            {
                char Msg1[] = "No socket is present. Aborting.\r\n";
                sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
                break;
            }

            char Msg1[] = "Select which socket you want to delete:\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);

            for (j = 0; j < udp_source_count_sockets(n); j++)
            {
                socket_port = udp_source_get_socket(n, j).local_port;
                len = sprintf(line, "%d.socket(local_port): %u\r\n", j + 1,
                              socket_port);
                sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
            }

            fetch_input(cmd_buf, CMD_BUFFER_SIZE);
            uint32_t sel = (uint32_t) atoi((const char*) cmd_buf); // 1 tabanlý seçim

            if (sel < 1 || sel > udp_source_count_sockets(n))
            {
                char Msg1[] = "Invalid Socket. Procedure not completed\r\n";
                sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
                break;
            }

            udp_sock_id_t chosen = udp_source_get_socket(n, sel - 1).socket_id; // numara -> id
            udp_source_remove_listener(chosen);    // id DEÐERLE geçilir (& YOK)

            const char Msg[] = "\r\nSocket has been succesfully deleted\r\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

            break;
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

        break;

    }

    case 3:
    {


        char MsgAsk[] ="What do you want to do?\r\n\n";
        sciSend(sciREGx, sizeof(MsgAsk) - 1, (uint8_t*) MsgAsk);
        char MsgAddNetif[] = "1.Add Netif\r\n";
        sciSend(sciREGx, sizeof(MsgAddNetif) - 1, (uint8_t*) MsgAddNetif);
        char MsgDeleteNetif[] = "2.Delete Netif\r\n";
        sciSend(sciREGx, sizeof(MsgDeleteNetif) - 1, (uint8_t*) MsgDeleteNetif);

        ch = read_choice_char();
        sciSendByte(sciREGx, ch);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        switch (ch)
        {

        //Adds a new netif interface with IP, Netmask and GW
        case '1':
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
            if (!(ipaddr_aton((const char*) cmd_buf, &new_ip)))
            {
                const char Msg[] = "Invalid value, netif not saved\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
                break;
            }

            const char PromptNetMask[] = "Netmask Address: ";
            sciSend(sciREGx, sizeof(PromptNetMask) - 1,
                    (uint8_t*) PromptNetMask);
            fetch_input(cmd_buf, CMD_BUFFER_SIZE);
            if (!(ipaddr_aton((const char*) cmd_buf, &new_netmask)))
            {
                const char Msg[] = "Invalid value, netif not saved\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
                break;
            }

            const char PromptGW[] = "GW Address: ";
            sciSend(sciREGx, sizeof(PromptGW) - 1, (uint8_t*) PromptGW);
            fetch_input(cmd_buf, CMD_BUFFER_SIZE);
            if (!(ipaddr_aton((const char*) cmd_buf, &new_gw)))
            {
                const char Msg[] = "Invalid value, netif not saved\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
                break;
            }

            if (ERR_OK == net_if_add(&new_ip, &new_netmask, &new_gw))
            {
                const char Msg[] =
                        "\r\nNew netif has been succesfully created\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
            }
            break;
        }

            //Deletes the chosen netif
            //Deletes all of its sockets as well
        case '2':
        {
            // code block

            struct netif *n;

            char Msg[] = "Please chose the interface you want to delete\r\n\n";
            sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);

            //writes the netif info(IP,Netmask,GW) for all the netifs
            n = terminal_select_netif();
            if (n == NULL)
                break;

            if (ERR_OK == net_if_remove(n))
            {
                const char Msg[] = "\r\nNetif has been succesfully deleted\r\n";
                sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
                break;
            }

            const char Err[] =
                    "There has been an error while deleting the netif\r\n";
            sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
            break;
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
        break;
    }

    //Send a UDP message of max length CMD_BUFFER_SIZE(32 byte)
    case 4:
    {
        uint8_t j;
        int len;
        struct netif *n;
        char line[96];

        char msg_netif[] ="Select the sender interface:\r\n\n";
        sciSend(sciREGx, sizeof(msg_netif) - 1, (uint8_t*) msg_netif);

        n = terminal_select_netif();
        if(NULL == n) break;

        char msg_port[] ="Chose the sending port\r\n";
        sciSend(sciREGx, sizeof(msg_port) - 1, (uint8_t*) msg_port);

        if(0 == udp_source_count_sockets(n))
        {
            char msg_port[] ="No active ports. Create a UDP socket first. Exiting...\r\n";
            sciSend(sciREGx, sizeof(msg_port) - 1, (uint8_t*) msg_port);
            break;
        }

        for (j = 0; j < udp_source_count_sockets(n); j++)
        {
            u16_t socket_port = udp_source_get_socket(n, j).local_port;
            len = sprintf(line, "%d.socket(local_port): %u\r\n", j + 1, socket_port);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }

        fetch_input(cmd_buf, CMD_BUFFER_SIZE);
        uint32_t sel = (uint32_t) atoi((const char*) cmd_buf); // 1 tabanlý seçim

        if (sel < 1 || sel > udp_source_count_sockets(n))
        {
            char Msg1[] = "Invalid Socket. Procedure not completed\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
            break;
        }
        udp_netif_socket_info_t socket = udp_source_get_socket(n, sel - 1);

        char Msg0[] ="Type the receiver IP address and port(192.168.0.1/5000): \r\n\n";
        sciSend(sciREGx, sizeof(Msg0) - 1, (uint8_t*) Msg0);

        fetch_input(cmd_buf, CMD_BUFFER_SIZE);

        // cmd_buf = "192.168.0.1/5000"
        char *sep = strchr((char*) cmd_buf, '/');   // '/' konumunu bul
        if (sep == NULL)
        {
            // "Invalid format (expected IP/port)" , hata
            char Msg1[] = "Invalid format, expected IP/port\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
            break;   // ya da uygun þekilde çýk
        }

        *sep = '\0';                        // '/' yerine null koy , string'i ikiye böl
        char *ip_str   = (char*) cmd_buf;   // "192.168.0.1"  (null'a kadar)
        char *port_str = sep + 1;           // "5000"         ('/'den sonrasý)

        ip_addr_t ip_rx;
        if (!ipaddr_aton(ip_str, &ip_rx))
        {
            // "Invalid IP" , hata
            char Msg1[] = "Invalid IP\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
            break;
        }

        u16_t port = (u16_t) atoi(port_str);
        if (port == 0)                      // atoi 0 = geçersiz/sayý deðil
        {
            // "Invalid port" , hata
            char Msg1[] = "Invalid Port\r\n";
            sciSend(sciREGx, sizeof(Msg1) - 1, (uint8_t*) Msg1);
            break;
        }

        char msg_data[] ="Enter the message: \r\n";
        sciSend(sciREGx, sizeof(msg_data) - 1, (uint8_t*) msg_data);

        fetch_input(cmd_buf, CMD_BUFFER_SIZE);

        if(ERR_OK != udp_source_data_send(socket.socket_id, socket.netif , &ip_rx, port, cmd_buf, (u16_t) strlen((char*) cmd_buf)))
        {
            char msg_fail[] ="Failed to send message\r\n";
            sciSend(sciREGx, sizeof(msg_fail) - 1, (uint8_t*) msg_fail);
            break;
        }

        char msg_success[] = "Message successfully sent\r\n";
        sciSend(sciREGx, sizeof(msg_success) - 1, (uint8_t*) msg_success);
        break;
    }


    //Sends a ICMP echo request to the IP address user has written
    case 5:
    {
        const char Msg[] = "Please chose which netif you want to ping from:";
        sciSend(sciREGx, sizeof(Msg) - 1, (uint8_t*) Msg);
        sciSendByte(sciREGx, '\r');
        sciSendByte(sciREGx, '\n');

        int len;
        struct netif *n;
        char ip_str[16];
        char line[96];
        ip_addr_t target;

       n = terminal_select_netif();
       if (n == NULL) break;

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

    //Resets the ARP table for all the netifs
    //Recommended after receiver changes their Ip addresses
    case 6:
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

    //Scans the whole network for open IP interfaces
    case 7:
    {
        struct netif *n;

        char msg_selection[] = "Select the interface to be used for scanning\r\n";
        sciSend(sciREGx, sizeof(msg_selection) - 1, (uint8_t*) msg_selection);

        n = terminal_select_netif();
        if(NULL == n) break;

        char msg_start[] = "Scanning the network for open interfaces...\r\n";
        sciSend(sciREGx, sizeof(msg_start) - 1, (uint8_t*) msg_start);

        if(ERR_OK != scan_network(n))
        {
            char msg_err[] = "Error while scanning\r\n";
            sciSend(sciREGx, sizeof(msg_err) - 1, (uint8_t*) msg_err);
        }
        char msg_success[] = "Scan Complete\r\n";
        sciSend(sciREGx, sizeof(msg_success) - 1, (uint8_t*) msg_success);
        break;
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



//reads the serial terminal line input and stores it inside the buffer
//default buffer is cmd_buf
void fetch_input(uint8_t *buf, uint32_t max_len)
{
    uint32_t len = 0;   // buf'taki karakter sayýsý
    uint32_t pos = 0;   // imleç konumu (0..len)
    uint32_t i;
    uint8_t  ch;

    while (sciIsRxReady(sciREGx)) { (void) sciReceiveByte(sciREGx); }  // bekleyenleri temizle

    for (;;)
    {
        sciReceive(sciREGx, 1, &ch);

        /* --- Enter: bitir --- */
        if (ch == '\r' || ch == '\n')
        {
            sciSendByte(sciREGx, '\r');
            sciSendByte(sciREGx, '\n');
            break;
        }

        /* --- Backspace: imleçten ÖNCEKÝ karakteri sil --- */
        else if (ch == 0x7F || ch == 0x08)
        {
            if (pos > 0)
            {
                for (i = pos - 1; i < len - 1; i++) buf[i] = buf[i + 1];  // sola kaydýr
                len--; pos--;

                sciSendByte(sciREGx, '\b');                      // imleci sola al
                sciSend(sciREGx, 3, (uint8_t*) "\x1b[K");        // satýr sonuna kadar sil
                if (len > pos) sciSend(sciREGx, len - pos, &buf[pos]);   // kuyruðu yeniden yaz
                for (i = 0; i < len - pos; i++) sciSendByte(sciREGx, '\b');  // imleci geri getir
            }
        }

        /* --- ESC: ok tuþlarý / Delete --- */
        else if (ch == 0x1B)
        {
            uint8_t seq;
            sciReceive(sciREGx, 1, &seq);            // '['
            if (seq == '[')
            {
                sciReceive(sciREGx, 1, &seq);        // yön/eylem baytý

                if (seq == 'D' && pos > 0)           // SOL ok
                {
                    pos--; sciSendByte(sciREGx, '\b');
                }
                else if (seq == 'C' && pos < len)    // SAÐ ok
                {
                    pos++; sciSend(sciREGx, 3, (uint8_t*) "\x1b[C");
                }
                else if (seq == '3')                 // Delete: ESC [ 3 ~
                {
                    sciReceive(sciREGx, 1, &seq);    // '~' yut
                    if (pos < len)
                    {
                        for (i = pos; i < len - 1; i++) buf[i] = buf[i + 1];
                        len--;
                        sciSend(sciREGx, 3, (uint8_t*) "\x1b[K");
                        if (len > pos) sciSend(sciREGx, len - pos, &buf[pos]);
                        for (i = 0; i < len - pos; i++) sciSendByte(sciREGx, '\b');
                    }
                }
                else if (seq >= '0' && seq <= '9')   // Home/End vb (n~) -> '~' yut, yoksay
                {
                    sciReceive(sciREGx, 1, &seq);
                }
                /* 'A'/'B' (yukarý/aþaðý) ve diðerleri: yoksay */
            }
        }

        /* --- Yazdýrýlabilir karakter: imleç konumuna EKLE --- */
        else if (len < max_len - 1)
        {
            for (i = len; i > pos; i--) buf[i] = buf[i - 1];   // saða kaydýr
            buf[pos] = ch;
            len++;

            sciSend(sciREGx, len - pos, &buf[pos]);            // yeni karakter + kuyruk
            pos++;
            for (i = 0; i < len - pos; i++) sciSendByte(sciREGx, '\b');  // ekleme sonrasýna dön
        }
        /* buffer dolu -> yoksay */
    }

    buf[len] = '\0';
}

//IMPORTANT: Check for NULL == n when using
//funciton returns NULL if user gives a wrong value
//lists all active netifs with their index number and returns the netif as a pointer value
//after user selects the index
static struct netif* terminal_select_netif(void)
{

    uint8_t ch;
    uint8_t i = 1;
    uint8_t j;
    u16_t socket_port;
    uint32_t idx = 0;
    int len;
    struct netif *n;
    char ip_str[16];
    char line[96];

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



    //reads the user input from 1-9 and selects the correct netif
    n = netif_list;
    ch = read_choice_char();
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
        return NULL;
    }
    return n;
}

//lists all the active netifs in the serial terminal
void terminal_list_netif(void)
{

    uint8_t i = 1;
    uint8_t j;
    u16_t socket_port;
    int len;
    struct netif *n;
    char ip_str[16];
    char line[96];

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
    return;
}

// ms kadar bekler; bu sýrada Ctrl+C (0x03) gelirse 1 döner (erken çýkýþ), yoksa 0.
static uint8_t wait_or_ctrlc(uint32_t ms)
{
    uint32_t start = rtiREG1->CNT[0U].FRCx;
    uint32_t ticks = ms * 9375U;   // RTI FRC0: 9.375 MHz -> 9375 tik/ms
    uint8_t  c;
    while ((rtiREG1->CNT[0U].FRCx - start) < ticks)
    {
        if (sciIsRxReady(sciREGx))
        {
            c = sciReceiveByte(sciREGx);
            if (c == TERM_CTRLC) return 1;
        }
    }
    return 0;
}

// "ping <IP>" komutunun gövdesi: IP string'ini alýr, kaynak netif seçtirir,
// PING_ATTEMPT_COUNT kez ping atar ve sonunda istatistik basar.
static void terminal_ping_command(const char *arg)
{
    struct netif *n;
    ip_addr_t target;
    char argbuf[32];
    char line[96];
    char tgt_str[16];
    int  len;
    int  attempt;
    uint8_t continuous = 0;
    char *sp;

    int   sent = 0, recv = 0;
    u32_t rtt_min = 0xFFFFFFFFu, rtt_max = 0, rtt_sum = 0;

    /* argümaný yerel tampona kopyala (üzerinde oynayacaðýz) */
    strncpy(argbuf, arg, sizeof(argbuf) - 1);
    argbuf[sizeof(argbuf) - 1] = '\0';

    /* "-t" bayraðý var mý? (kesmeden ÖNCE ara) */
    if (strstr(argbuf, "-t") != NULL) continuous = 1;

    /* IP'yi ilk boþlukta kes -> argbuf sadece IP kalsýn */
    sp = strchr(argbuf, ' ');
    if (sp != NULL) *sp = '\0';

    if (!ipaddr_aton(argbuf, &target))
    {
        const char Err[] = "Invalid IP address\r\n";
        sciSend(sciREGx, sizeof(Err) - 1, (uint8_t*) Err);
        return;
    }

    const char MsgSel[] = "Choose netif to send ping from:\r\n";
    sciSend(sciREGx, sizeof(MsgSel) - 1, (uint8_t*) MsgSel);
    n = terminal_select_netif();
    if (n == NULL) return;

    ipaddr_ntoa_r(&target, tgt_str, sizeof(tgt_str));
    if (continuous)
        len = snprintf(line, sizeof(line), "\r\nPinging %s (Ctrl+C to stop):\r\n", tgt_str);
    else
        len = snprintf(line, sizeof(line), "\r\nPinging %s:\r\n", tgt_str);
    sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

    /* continuous ise sonsuz, deðilse PING_ATTEMPT_COUNT kez */
    for (attempt = 0; continuous || attempt < PING_ATTEMPT_COUNT; attempt++)
    {
        ping_result_t res;
        err_t perr;

        sent++;
        perr = ping_send(n, &target);
        if (perr != ERR_OK)
        {
            len = snprintf(line, sizeof(line), "Ping send err: %d\r\n", (int) perr);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }
        else if (ping_wait_reply(PING_TIMEOUT_MS, &res))
        {
            char from_str[16];
            recv++;
            rtt_sum += res.rtt_ms;
            if (res.rtt_ms < rtt_min) rtt_min = res.rtt_ms;
            if (res.rtt_ms > rtt_max) rtt_max = res.rtt_ms;

            ipaddr_ntoa_r(&res.from, from_str, sizeof(from_str));
            len = snprintf(line, sizeof(line),
                    "Reply from %s: seq=%u bytes=%u time=%lums TTL=%u\r\n",
                    from_str, (unsigned) res.seqno, (unsigned) res.data_len,
                    (unsigned long) res.rtt_ms, (unsigned) res.ttl);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }
        else
        {
            const char Tmo[] = "Request timed out\r\n";
            sciSend(sciREGx, sizeof(Tmo) - 1, (uint8_t*) Tmo);
        }

        /* sürekli modda: ~1 sn beklerken Ctrl+C'yi de dinle */
        if (continuous)
        {
            if (wait_or_ctrlc(1000)) break;   // Ctrl+C -> döngüden çýk
        }
    }

    /* istatistikler (her iki modda da) */
    {
        int loss = (sent > 0) ? ((sent - recv) * 100 / sent) : 0;

        len = snprintf(line, sizeof(line),
                "\r\n--- %s ping statistics ---\r\n", tgt_str);
        sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

        len = snprintf(line, sizeof(line),
                "%d transmitted, %d received, %d%% packet loss\r\n",
                sent, recv, loss);
        sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

        if (recv > 0)
        {
            len = snprintf(line, sizeof(line),
                    "rtt min/avg/max = %lu/%lu/%lu ms\r\n",
                    (unsigned long) rtt_min,
                    (unsigned long) (rtt_sum / recv),
                    (unsigned long) rtt_max);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }
    }
}


/* ============================================================
 *  Yapisal komut kanali (Protokol B)
 *  Ana dongu 'q' disindaki baytlari buraya besler; '\r' gelince
 *  satir bir komut olarak islenir ve '#'-cerceveli yanit basilir.
 *
 *  Komutlar:
 *    NETIF LIST
 *    NETIF ADD <ip> <mask> <gw>
 *    NETIF DEL <idx>
 *    SOCK OPEN <idx> <port>
 *    SOCK CLOSE <idx> <nth>
 * ============================================================ */

#define CMD_LINE_MAX 128
static char     cmd_line[CMD_LINE_MAX];
static uint16_t cmd_line_len = 0;

/* null-sonlu string'i seri porta yolla */
static void cmd_out(const char *s)
{
    sciSend(sciREGx, (uint32_t) strlen(s), (uint8_t*) s);
}

/* 1 tabanli indeks -> netif*  (bulunamazsa NULL) */
static struct netif* cmd_netif_by_index(int idx)
{
    struct netif *n;
    if (idx < 1) return NULL;
    for (n = netif_list; n != NULL && idx > 1; n = n->next) idx--;
    return n;
}

/* NETIF LIST -> her netif icin #NETIF satiri + soketleri icin #SOCK satirlari */
static void cmd_netif_list(void)
{
    struct netif *n;
    int idx = 1;
    int len;
    uint8_t j;
    char line[96];
    char ip_str[16], mask_str[16], gw_str[16];

    cmd_out("#BEGIN NETIFLIST\r\n");

    for (n = netif_list; n != NULL; n = n->next)
    {
        ipaddr_ntoa_r(&n->ip_addr, ip_str,   sizeof(ip_str));
        ipaddr_ntoa_r(&n->netmask, mask_str, sizeof(mask_str));
        ipaddr_ntoa_r(&n->gw,      gw_str,   sizeof(gw_str));

        len = snprintf(line, sizeof(line),
                       "#NETIF %d %s %s %s socks=%u\r\n",
                       idx, ip_str, mask_str, gw_str,
                       (unsigned) udp_source_count_sockets(n));
        sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);

        for (j = 0; j < udp_source_count_sockets(n); j++)
        {
            len = snprintf(line, sizeof(line), "#SOCK %d %u %u\r\n",
                           idx, (unsigned) (j + 1),
                           (unsigned) udp_source_get_socket(n, j).local_port);
            sciSend(sciREGx, (uint32_t) len, (uint8_t*) line);
        }
        idx++;
    }

    cmd_out("#END NETIFLIST OK\r\n");
}

/* NETIF ADD <ip> <mask> <gw> */
static void cmd_netif_add(const char *args)
{
    char ip_s[16], mask_s[16], gw_s[16];
    ip_addr_t ip, mask, gw;

    cmd_out("#BEGIN NETIFADD\r\n");

    if (sscanf(args, "%15s %15s %15s", ip_s, mask_s, gw_s) != 3
        || !ipaddr_aton(ip_s, &ip)
        || !ipaddr_aton(mask_s, &mask)
        || !ipaddr_aton(gw_s, &gw))
    {
        cmd_out("#END NETIFADD ERR:bad_args\r\n");
        return;
    }

    if (ERR_OK != net_if_add(&ip, &mask, &gw))
    {
        cmd_out("#END NETIFADD ERR:add_failed\r\n");
        return;
    }
    cmd_out("#END NETIFADD OK\r\n");
}

/* NETIF DEL <idx> */
static void cmd_netif_del(const char *args)
{
    struct netif *n = cmd_netif_by_index(atoi(args));

    cmd_out("#BEGIN NETIFDEL\r\n");

    if (NULL == n)
    {
        cmd_out("#END NETIFDEL ERR:bad_index\r\n");
        return;
    }
    if (ERR_OK != net_if_remove(n))
    {
        cmd_out("#END NETIFDEL ERR:remove_failed\r\n");
        return;
    }
    cmd_out("#END NETIFDEL OK\r\n");
}

/* SOCK OPEN <idx> <port> */
static void cmd_sock_open(const char *args)
{
    int idx = 0, port = 0;
    udp_sock_id_t id;
    struct netif *n;

    cmd_out("#BEGIN SOCKOPEN\r\n");

    if (sscanf(args, "%d %d", &idx, &port) != 2 || port < 1 || port > 65535)
    {
        cmd_out("#END SOCKOPEN ERR:bad_args\r\n");
        return;
    }
    n = cmd_netif_by_index(idx);
    if (NULL == n)
    {
        cmd_out("#END SOCKOPEN ERR:bad_index\r\n");
        return;
    }
    if (ERR_OK != udp_source_add_listener(n, (u16_t) port, &id))
    {
        cmd_out("#END SOCKOPEN ERR:open_failed\r\n");
        return;
    }
    cmd_out("#END SOCKOPEN OK\r\n");
}

/* SOCK CLOSE <idx> <nth> */
static void cmd_sock_close(const char *args)
{
    int idx = 0, nth = 0;
    struct netif *n;

    cmd_out("#BEGIN SOCKCLOSE\r\n");

    if (sscanf(args, "%d %d", &idx, &nth) != 2)
    {
        cmd_out("#END SOCKCLOSE ERR:bad_args\r\n");
        return;
    }
    n = cmd_netif_by_index(idx);
    if (NULL == n)
    {
        cmd_out("#END SOCKCLOSE ERR:bad_index\r\n");
        return;
    }
    if (nth < 1 || nth > udp_source_count_sockets(n))
    {
        cmd_out("#END SOCKCLOSE ERR:bad_socket\r\n");
        return;
    }
    udp_source_remove_listener(udp_source_get_socket(n, (u8_t)(nth - 1)).socket_id);
    cmd_out("#END SOCKCLOSE OK\r\n");
}

/* tam bir komut satirini isle */
static void cmd_dispatch(char *line)
{
    if      (strcmp (line, "NETIF LIST") == 0)      cmd_netif_list();
    else if (strncmp(line, "NETIF ADD ", 10) == 0)  cmd_netif_add(line + 10);
    else if (strncmp(line, "NETIF DEL ", 10) == 0)  cmd_netif_del(line + 10);
    else if (strncmp(line, "SOCK OPEN ", 10) == 0)  cmd_sock_open(line + 10);
    else if (strncmp(line, "SOCK CLOSE ", 11) == 0) cmd_sock_close(line + 11);
    else                                            cmd_out("#ERR unknown_command\r\n");
}

/* ana dongunun besledigi bayt akisi; '\r'/'\n' gelince satiri isle */
void cmd_channel_feed(uint8_t ch)
{
    if (ch == '\r' || ch == '\n')
    {
        if (cmd_line_len > 0)
        {
            cmd_line[cmd_line_len] = '\0';
            cmd_dispatch(cmd_line);
            cmd_line_len = 0;
        }
        return;
    }

    if (cmd_line_len < CMD_LINE_MAX - 1)
    {
        cmd_line[cmd_line_len++] = (char) ch;
    }
    /* satir tasarsa fazlasini yoksay (sonraki '\r'de sifirlanir) */
}
