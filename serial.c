/*
 * Sunrise Systems NXTP Transit Sign Driver
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

#include "common.h"
#include "serial.h"

/* workaround for CRTSCTS not being defined on some platforms */
#ifndef CRTSCTS
#define CRTSCTS	020000000000 /* flow control */
#endif

int8_t set_port_attrs(struct serialport_t *port_obj, uint32_t speed) {
	struct termios tty;

	memset(&tty, 0, sizeof(struct termios));

	if (tcgetattr(port_obj->fd, &tty) != 0) {
		fprintf(stderr, "error from tcgetattr: %s\n", strerror(errno));
		return -1;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	tty.c_cflag |= (CLOCAL | CREAD);	/* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;			/* 8-bit characters */
	tty.c_cflag &= ~PARENB;			/* no parity bit */
	tty.c_cflag &= ~CSTOPB;			/* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;		/* no hardware flow control */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR);
	tty.c_iflag &= ~(ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(port_obj->fd, TCSANOW, &tty) != 0) {
		fprintf(stderr, "error from tcsetattr: %s\n", strerror(errno));
		return -1;
	}

	return 1;
}

int8_t serial_open_port(struct serialport_t *port_obj, char *port) {

	memset(port_obj, 0, sizeof(struct serialport_t));
	strncpy(port_obj->port, port, 32);

	port_obj->fd = open(port_obj->port, O_RDWR | O_NOCTTY | O_SYNC);
	if (port_obj->fd < 0) {
		fprintf(stderr, "error opening %s: %d (%s)\n",
			port_obj->port, port_obj->fd, strerror(errno));
		return -1;
	}

	/* set speed to 9600 baud, 8n1 */
	return set_port_attrs(port_obj, B9600);
}

uint8_t serial_send(struct serialport_t *port_obj,
	char *data, uint8_t len) {
	return write(port_obj->fd, data, len) < 0 ? -1 : 1;
}

uint8_t serial_receive(struct serialport_t *port_obj,
	char *data, uint8_t *len) {
	char buf[128];

	/* read up to 128 characters if ready to read */
	*len = read(port_obj->fd, buf, 128);
	memcpy(data, buf, *len);

	return 1;
}

int8_t serial_close_port(struct serialport_t *port_obj) {
	return close(port_obj->fd) < 0 ? -1 : 1;
}
