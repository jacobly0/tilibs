/*  libticalcs - calculator library, a part of the TiLP project
 *  Copyright (C) 1999-2003  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CMDS_82__
#define __CMDS_82__


int ti82_send_VAR(uint16_t varsize, uint8_t vartype, char *varname);
int ti82_send_CTS(void);
int ti82_send_XDP(int length, uint8_t * data);
int ti82_send_SKIP(uint8_t rej_code);
int ti82_send_ACK(void);
int ti82_send_ERR(void);
int ti82_send_SCR(void);
int ti82_send_EOT(void);
int ti82_send_REQ(uint16_t varsize, uint8_t vartype, char *varname);
int ti82_send_RTS(uint16_t varsize, uint8_t vartype, char *varname);

int ti82_recv_VAR(uint16_t * varsize, uint8_t * vartype, char *varname);
int ti82_recv_CTS(void);
int ti82_recv_SKIP(uint8_t * rej_code);
int ti82_recv_XDP(uint16_t * length, uint8_t * data);
int ti82_recv_ACK(uint16_t * status);
int ti82_recv_RTS(uint16_t * varsize, uint8_t * vartype, char *varname);


#endif
