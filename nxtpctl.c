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
#include "packet.h"
#include "serial.h"
#include "text.h"

int main(/*int argc, char *argv[]*/) {
	int ret;
	char buf[1024];
	uint8_t buf_len;
	char *port = "/dev/ttyUSB0";
	struct serialport_t my_port;
	struct ctlr_cfg_t ctlr;

	/* sign controller configuration */
	set_ctlr_config(&ctlr, 195, 255, 245);

	ret = serial_open_port(&my_port, port);
	if (ret < 0) {
		fprintf(stderr, "couldn't open port.\n");
		return 1;
	}

	scrolling_text(ctlr, "VERNON & LONG BEACH & RAIL A", buf, &buf_len);
	serial_send(&my_port, buf, buf_len);
	sleep(4);

	reset_sign(ctlr, buf, &buf_len);
	serial_send(&my_port, buf, buf_len);
	sleep(3);

	for (uint8_t i = 0; i < 5; i++) {
		static_text(ctlr, "STOP REQUESTED", 5, buf, &buf_len);
		serial_send(&my_port, buf, buf_len);
		sleep(8);

		scrolling_text(ctlr, "PLEASE USE REAR EXIT", buf, &buf_len);
		serial_send(&my_port, buf, buf_len);
		sleep(8);
	}

	ret = serial_close_port(&my_port);
	if (ret < 0) {
		fprintf(stderr, "couldn't close port.\n");
	}

	return 0;
}
