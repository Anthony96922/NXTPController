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
		"Sunrise Systems NXTP Sign Controller v" VERSION "\n"
		"\n"
		"Usage: %s [ -p port ] -a address\n"
		"\t\t-t text [ -s scroll-value ]\n"
		"\n"
		"\t-p port\t\tUART port to use (default: \"%s\")\n"
		"\t-a address\tAddress of one or more signs\n"
		"\t-t text\t\tText string to use\n"
		"\t-s scroll-value\tScroll value to use\n"
		"\t-h\t\tShow this help and exit\n"
		"\t-v\t\tShow version and exit\n"
		"\n",
	name, DEFAULT_PORT);
}

int main(int argc, char *argv[]) {
	int opt;
	char text[MAX_TEXT_LEN];
	char data_buf[BUF_LEN];
	uint8_t data_buf_len;
	char port[PORT_SIZE];
	uint8_t port_set = 0;
	struct serialport_t my_port;
	struct ctlr_cfg_t my_ctlr;
	uint8_t address = 0; /* default to all signs */
	uint8_t scroll = 0;
	uint8_t text_set = 0;

	const char *short_opt = "p:a:t:s:hv";
	const struct option long_opt[] = {
		{"port",	required_argument,	NULL,	'p'},
		{"address",	required_argument,	NULL,	'a'},
		{"text",	required_argument,	NULL,	't'},
		{"scroll",	required_argument,	NULL,	's'},
		{"help",	no_argument,		NULL,	'h'},
		{0,		0,			0,	0}
	};


	memset(port, 0, PORT_SIZE);
	memset(text, 0, MAX_TEXT_LEN);

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
	{
		switch (opt) {
			case 'p':
				strncpy(port, optarg, PORT_SIZE - 1);
				printf("Using serial port \"%s\".\n", port);
				port_set = 1;
				break;
			case 'a':
				address = strtoul(optarg, NULL, 16) & 255;
				printf("Using address %u.\n", address);
				break;
			case 't':
				strncpy(text, optarg, MAX_TEXT_LEN - 1);
				printf("Using text \"%s\".\n", text);
				text_set = 1;
				break;
			case 's':
				scroll = strtoul(optarg, NULL, 16);
				break;
			case 'v':
				printf(VERSION "\n");
				return 0;
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
	reset_sign(my_ctlr, address, data_buf, &data_buf_len);
	serial_put_buffer(&my_port, data_buf, data_buf_len);
	serial_send(&my_port);

#if 1
	if (text_set) {
		if (scroll) {
			scrolling_text(my_ctlr,
				address, text, scroll,
				data_buf, &data_buf_len);
		} else {
			static_text(my_ctlr,
				address, text, 5, data_buf, &data_buf_len);
		}
	}

	serial_put_buffer(&my_port, data_buf, data_buf_len);
	serial_send(&my_port);
	sleep(1);

#else

	scrolling_text(my_ctlr,
		address, "ARCADIA GOLD LINE STATION & RAIL G",
		4, data_buf, &data_buf_len);
	serial_put_buffer(&my_port, data_buf, data_buf_len);
	serial_send(&my_port);
	sleep(5);

	reset_sign(my_ctlr, address, data_buf, &data_buf_len);
	serial_put_buffer(&my_port, data_buf, data_buf_len);
	serial_send(&my_port);
	sleep(2);

	/* stop requested routine */
	for (uint8_t i = 0; i < 3; i++) {
		static_text(my_ctlr,
			address, "STOP REQUESTED", 5,
			data_buf, &data_buf_len);
		serial_put_buffer(&my_port, data_buf, data_buf_len);
		serial_send(&my_port);
		sleep(7);

		scrolling_text(my_ctlr,
			address, "PLEASE USE REAR EXIT", 5,
			data_buf, &data_buf_len);
		serial_put_buffer(&my_port, data_buf, data_buf_len);
		serial_send(&my_port);
		sleep(7);
	}
#endif

	serial_close_port(&my_port);

	return 0;
}
