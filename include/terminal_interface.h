/*
 * terminal_interface.h
 *
 *  Created on: 22 Tem 2026
 *      Author: PC_4434
 */

#ifndef INCLUDE_TERMINAL_INTERFACE_H_
#define INCLUDE_TERMINAL_INTERFACE_H_

#include "HL_sci.h"
#include "ipv4/lwip/ip_addr.h"

#define CMD_BUFFER_SIZE 32

extern volatile uint8_t terminal_input_flag;
extern uint8_t cmd_buf[CMD_BUFFER_SIZE];

void read_terminal_line(void);

void fetch_input(uint8_t *buf, uint32_t max_len);

static struct netif* terminal_select_netif(void);

void terminal_list_netif(void);

static void terminal_ping_command(const char *ip_str);

/* --- Yapisal komut kanali ---
 * Ana dongu 'q' disindaki her bayti cmd_channel_feed()'e verir;
 * '\r' gelince satir komut olarak islenir. Bloklamaz, yankilamaz.
 *
 * Yanit cercevesi:
 *   #BEGIN <AD>
 *   ...veri satirlari...
 *   #END <AD> OK          (hata durumunda: #END <AD> ERR:<sebep>)
 *
 * Komutlar:
 *   NETIF LIST | NETIF ADD <ip> <mask> <gw> | NETIF DEL <idx>
 *   NETIF SET <idx> IP|MASK|GW <deger>
 *   SOCK OPEN <idx> <port> | SOCK CLOSE <idx> <nth>
 *   PING <idx> <ip> | SCAN <idx> | ARPRESET
 *   UDP SEND <idx> <nth> <ip> <port> <veri>
 * <idx> ve <nth> 1 tabanlidir (NETIF LIST ciktisindaki sira).
 *
 * Istem disi bildirimler #EVT onekiyle basilir; bir yanitin ortasina
 * dusebilir, yanitin parcasi degildir.
 *
 * Yeni komut eklerken: her cikis yolu bir #END satiri yazmali,
 * yoksa karsi taraf zaman asimina kadar bekler. */
void cmd_channel_feed(uint8_t ch);

#endif /* INCLUDE_TERMINAL_INTERFACE_H_ */
