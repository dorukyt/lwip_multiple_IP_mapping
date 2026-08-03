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
#define MAX_DEST 8

extern volatile uint8_t terminal_input_flag;
extern uint8_t cmd_buf[CMD_BUFFER_SIZE];

typedef struct dest_node
{
    ip_addr_t dest_addr;
    uint8_t in_use;
    struct dest_node *next;
}dest_node_t;

extern dest_node_t *dest_list_head;

dest_node_t *dest_list_add(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

void read_terminal_line(void);

void fetch_input(uint8_t *buf, uint32_t max_len);

static struct netif* terminal_select_netif(void);

static void terminal_list_netif(void);

#endif /* INCLUDE_TERMINAL_INTERFACE_H_ */
