/*
 * Sunrise Systems NXTP Transit Sign Controller
 * Copyright (C) 2022 Anthony96922
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#define PORT_SIZE	32
#define BUF_LEN		512

/* serial port object */
typedef struct serialport_t {
	char port[PORT_SIZE];
	int fd;
	char buf[BUF_LEN];
	uint16_t buf_len;
} serialport_t;

/* workaround for CRTSCTS not being defined */
#ifndef CRTSCTS
#define CRTSCTS	020000000000 /* flow control */
#endif

extern int8_t serial_open_port(struct serialport_t *port_obj, char *port);
extern void serial_put_buffer(struct serialport_t *port_obj,
	char *data, uint16_t len);
extern void serial_get_buffer(struct serialport_t *port_obj,
	char *data, uint16_t *len);
extern int8_t serial_send(struct serialport_t *port_obj);
extern int8_t serial_receive(struct serialport_t *port_obj);
extern int8_t serial_close_port(struct serialport_t *port_obj);
