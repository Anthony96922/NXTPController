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

/*
 * code for handling serial I/O
 *
 * parts of this code came from
 * https://stackoverflow.com/questions/57152937/canonical-mode-linux-serial-port/57155531#57155531
 */

#include "common.h"
#include "packet.h"
#include "serial.h"

int8_t serial_open_port(struct serialport_t *port_obj, char *port) {
	struct termios tty;

	memset(port_obj, 0, sizeof(struct serialport_t));
	strncpy(port_obj->port, port, PORT_SIZE);

	/* open sesame */
	port_obj->fd = open(port_obj->port, O_RDWR | O_NOCTTY | O_SYNC);
	if (port_obj->fd < 0) {
		fprintf(stderr, "(%s): Error opening %s: %d (%s)\n",
			__func__, port_obj->port, -errno, strerror(errno));
		return -1;
	}

	memset(&tty, 0, sizeof(struct termios));

	if (tcgetattr(port_obj->fd, &tty) != 0) {
		fprintf(stderr, "(%s): Error from tcgetattr: %d (%s)\n",
			__func__, -errno, strerror(errno));
		return -1;
	}

	/* set speed to 9600 baud, 8n1 */
	cfsetospeed(&tty, B9600);
	cfsetispeed(&tty, B9600);

	tty.c_cflag |= CLOCAL | CREAD;	/* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;		/* 8-bit characters */
	tty.c_cflag &= ~PARENB;		/* no parity bit */
	tty.c_cflag &= ~CSTOPB;		/* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;	/* no hardware flow control */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR);
	tty.c_iflag &= ~(ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(port_obj->fd, TCSANOW, &tty) != 0) {
		fprintf(stderr, "(%s): Error from tcsetattr: %d (%s)\n",
			__func__, -errno, strerror(errno));
		return -1;
	}

	return 1;
}

void serial_put_buffer(struct serialport_t *port_obj,
	struct data_buf_t data_buf) {
	/* buffer overflow protection */
	if (data_buf.len > BUF_LEN) return;
	memcpy(port_obj->buf + port_obj->buf_len,
		data_buf.data, data_buf.len);
	port_obj->buf_len += data_buf.len;
}

void serial_get_buffer(struct serialport_t *port_obj,
	struct data_buf_t *data_buf) {
	memcpy(data_buf->data, port_obj->buf, port_obj->buf_len);
	data_buf->len = port_obj->buf_len;
}

/*
 * reset the buffer state
 *
 */
static void serial_reset_buffer(struct serialport_t *port_obj) {
	memset(port_obj->buf, 0, BUF_LEN);
	port_obj->buf_len = 0;
}

int8_t serial_send(struct serialport_t *port_obj) {
	int16_t ret;

	/* return when there is nothing to send */
	if (!port_obj->buf_len) {
		fprintf(stderr, "(%s): Nothing to send!\n", __func__);
		return -1;
	}

	/* write up to 512 characters */
	ret = write(port_obj->fd, port_obj->buf, port_obj->buf_len);
	if (ret < 0) {
		fprintf(stderr, "(%s): Couldn't send: %d (%s)\n",
			__func__, -errno, strerror(errno));
		return -1;
	}

	/* wait for sending to finish */
	tcdrain(port_obj->fd);
	/* reset internal buffer when done */
	serial_reset_buffer(port_obj);

	return 1;
}

int8_t serial_receive(struct serialport_t *port_obj) {
	int16_t ret;

	/* prepare internal buffer for new data */
	serial_reset_buffer(port_obj);

	/* read up to 512 characters */
	ret = read(port_obj->fd, port_obj->buf, BUF_LEN);
	if (ret < 0) {
		fprintf(stderr, "(%s): Couldn't receive: %d (%s)\n",
			__func__, -errno, strerror(errno));
		return -1;
	}

	/* set actual number of bytes when done */
	port_obj->buf_len = ret;

	return 1;
}

int8_t serial_close_port(struct serialport_t *port_obj) {

	if (close(port_obj->fd) < 0) {
		fprintf(stderr, "(%s): Error closing %s: %d (%s)\n",
			__func__, port_obj->port, -errno, strerror(errno));
		return -1;
	}

	return 1;
}
