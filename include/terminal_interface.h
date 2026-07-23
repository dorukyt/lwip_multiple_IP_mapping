/*
 * terminal_interface.h
 *
 *  Created on: 22 Tem 2026
 *      Author: PC_4434
 */

#ifndef INCLUDE_TERMINAL_INTERFACE_H_
#define INCLUDE_TERMINAL_INTERFACE_H_

#include "HL_sci.h"

#define CMD_BUFFER_SIZE 32

extern volatile uint8_t terminal_input_flag;
extern uint8_t cmd_buf[CMD_BUFFER_SIZE];

void read_terminal_line(void);

void fetch_input(uint8_t *buf, uint32_t max_len);

#endif /* INCLUDE_TERMINAL_INTERFACE_H_ */
