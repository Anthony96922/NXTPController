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

#define DEFAULT_PORT	"/dev/ttyUSB0"

static void show_help(char *name) {
	fprintf(stderr,
		"Sunrise Systems NXTP Sign Controller\n"
		"\n"
		"Usage: %s -t text [ -s scroll-value ]\n"
		"\n"
		"\t-t text\t\tText item to use\n"
		"\t-s scroll-value\tScroll value to use\n"
		"\n",
	name);
}

int main(int argc, char *argv[]) {
	int opt;
	char buf[MAX_TEXT_LEN*2];
	char text[MAX_TEXT_LEN];
	uint8_t buf_len;
	char port[SERIAL_PORT_SIZE];
	uint8_t port_set = 0;
	struct serialport_t my_port;
	struct ctlr_cfg_t my_ctlr;
	uint8_t scroll = 0;
	uint8_t text_set = 0;

	const char *short_opt = "p:t:s:h";
	const struct option long_opt[] = {
		{"port",	required_argument,	NULL,	'p'},
		{"text",	required_argument,	NULL,	't'},
		{"scroll",	required_argument,	NULL,	's'},
		{"help",	no_argument,		NULL,	'h'},
		{0,		0,			0,	0}
	};


	memset(port, 0, SERIAL_PORT_SIZE);
	memset(text, 0, MAX_TEXT_LEN);

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
			case 'p':
				strncpy(port, optarg, SERIAL_PORT_SIZE - 1);
				printf("Using serial port \"%s\".\n", port);
				port_set = 1;
				break;
			case 't':
				strncpy(text, optarg, MAX_TEXT_LEN - 1);
				printf("Using text \"%s\".\n", text);
				text_set = 1;
				break;
			case 's':
				scroll = strtoul(optarg, NULL, 16);
				break;
			case 'h':
			case '?':
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	if (!port_set) {
		strcpy(port, DEFAULT_PORT);
		printf("Using default port \"%s\".\n", port);
	}

	/* sign controller configuration */
	set_ctlr_config(&my_ctlr, 195, 255, 245);

	/* open the serial port (9600 8n1) */
	if (serial_open_port(&my_port, port) < 0) return 1;

	/* reset the sign */
	reset_sign(my_ctlr, 1, buf, &buf_len);
	serial_send(&my_port, buf, buf_len);

	if (text_set) {
		if (scroll) {
			scrolling_text(my_ctlr,
				1, text, scroll, buf, &buf_len);
		} else {
			static_text(my_ctlr,
				1, text, 5, buf, &buf_len);
		}
	}

	serial_send(&my_port, buf, buf_len);
	sleep(1);

#if 0
	scrolling_text(my_ctlr,
		1, "ARCADIA GOLD LINE STATION & RAIL G", 4, buf, &buf_len);
	serial_send(&my_port, buf, buf_len);
	sleep(5);

	reset_sign(my_ctlr, 1, buf, &buf_len);
	serial_send(&my_port, buf, buf_len);
	sleep(2);

	/* stop requested routine */
	for (uint8_t i = 0; i < 3; i++) {
		static_text(my_ctlr,
			1, "STOP REQUESTED", 5, buf, &buf_len);
		serial_send(&my_port, buf, buf_len);
		sleep(8);

		scrolling_text(my_ctlr,
			1, "PLEASE USE REAR EXIT", 5, buf, &buf_len);
		serial_send(&my_port, buf, buf_len);
		sleep(8);
	}
#endif

	if (serial_close_port(&my_port) < 0) {
		fprintf(stderr, "couldn't close port.\n");
	}

	return 0;
}
