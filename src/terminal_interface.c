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
#include "terminal_interface.h"

#define sciREGx sciREG1
uint8_t cmd_buf[CMD_BUFFER_SIZE];

void read_terminal_line(uint8_t *buf, uint32_t max_len)
{

    uint32_t idx = 0;
    uint8_t ch;

    while(sciIsRxReady(sciREGx))
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
