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
		"Usage: %s [ -p port ] -a address|addr1,addr2\n"
		"\t\t-t text [ -f fmt-name,fmt-value ... ]\n"
		"\n"
		"\t-p port\t\tUART port to use (default: \"%s\")\n"
		"\t-a address\tAddress of one or more signs\n"
		"\t-t text\t\tText string to use\n"
		"\t-f name,value\tFormat name and value\n"
		"\t-r\t\tRailroad X-ing mode\n"
		"\t-h\t\tShow this help and exit\n"
		"\t-v\t\tShow version and exit\n"
		"\n",
	name, DEFAULT_PORT);
}

int main(int argc, char *argv[]) {
	int opt;
	char text[MAX_TEXT_LEN + 1];
	struct data_buf_t data_buf;
	char port[PORT_SIZE];
	uint8_t port_set = 0;
	struct serialport_t my_port;
	struct ctlr_cfg_t my_ctlr;
	uint8_t address[2] = {0}; /* default to all signs */
	struct text_fmt_t fmt[5];
	uint8_t fmt_idx = 0;
	uint8_t text_set = 0;
	uint8_t rrxing = 0;

	const char *short_opt = "p:a:t:f:rhv";
	const struct option long_opt[] = {
		{"port",	required_argument,	NULL,	'p'},
		{"address",	required_argument,	NULL,	'a'},
		{"text",	required_argument,	NULL,	't'},
		{"format",	required_argument,	NULL,	's'},

		/* preset functions */
		{"rrxing", 	required_argument,	NULL,	128},

		{"help",	no_argument,		NULL,	'h'},
		{0,		0,			0,	0}
	};

	address[0] = 1;

	memset(port, 0, PORT_SIZE);
	memset(text, 0, MAX_TEXT_LEN + 1);

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
	{
		switch (opt) {
			case 'p':
				strncpy(port, optarg, PORT_SIZE - 1);
				printf("Using serial port \"%s\".\n", port);
				port_set = 1;
				break;
			case 'a':
				if (sscanf(optarg, "%hhu,%hhu",
					&address[0], &address[1]) == 2) {
					printf("Configured addresses %u and %u"
						" for railroad crossing"
						" mode.\n",
						address[0], address[1]);
				} else {
					address[0] = strtoul(optarg, NULL, 16);
					printf("Using address %u.\n",
						address[0]);
				}
				break;
			case 't':
				strncpy(text, optarg, MAX_TEXT_LEN);
				printf("Using text \"%s\".\n", text);
				text_set = 1;
				break;
			case 'f':
				if (sscanf(optarg, "%c,%hhu",
					&fmt[fmt_idx].name,
					&fmt[fmt_idx].value) == 2) {
					printf("Using format '%c' = %u.\n",
						fmt[fmt_idx].name,
						fmt[fmt_idx].value);
					if (fmt_idx < 5) fmt_idx++;
				} else if (sscanf(optarg, "%c,%c",
					&fmt[fmt_idx].name,
					&fmt[fmt_idx].value) == 2) {
					printf("Using format '%c' = '%c'.\n",
						fmt[fmt_idx].name,
						fmt[fmt_idx].value);
					if (fmt_idx < 5) fmt_idx++;
				} else {
					printf("Invalid format syntax.\n");
				}
				break;

			case 'r':
				rrxing = 1;
				printf("Activated railroad crossing mode.\n");
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
	make_reset_packet(my_ctlr, &data_buf, address[0]);
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);

	if (rrxing) {
		uint8_t k = 0;

		for (uint8_t i = 0; i < 30; i++) {
			k ^= 1;
			for (uint8_t j = 0; j < 2; j++) {
				make_text(my_ctlr, &data_buf,
						j ? address[1] : address[0],
						j ?
							(k ? "^Y12" : "^Y13") :
							(k ? "^Y13" : "^Y12")
				);
				serial_put_buffer(&my_port, data_buf);
				serial_send(&my_port);
				reset_data_buf(&data_buf);

				make_trigger_packet(my_ctlr, &data_buf);
				serial_put_buffer(&my_port, data_buf);
				serial_send(&my_port);
				reset_data_buf(&data_buf);

			}

			usleep(500000);
		}

		/* reset when finished */
		for (uint8_t j = 0; j < 2; j++) {
			make_reset_packet(my_ctlr, &data_buf,
				j ? address[1] : address[0]);
			serial_put_buffer(&my_port, data_buf);
			serial_send(&my_port);
			reset_data_buf(&data_buf);
		}

		goto exit;
	}

#if 1
	if (text_set) {
		make_text(my_ctlr, &data_buf,
			address[0], text);

		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);
	}

	for (uint8_t i = 0; i < fmt_idx; i++) {
		make_format_packet(my_ctlr, &data_buf, fmt[i]);

		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);
	}

	make_trigger_packet(my_ctlr, &data_buf);
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);
#else
	struct text_fmt_t tmp_fmt;
	make_text(my_ctlr, &data_buf,
		address[0], "ARCADIA GOLD LINE STATION & RAIL G");
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);

	tmp_fmt.name = 'S';
	tmp_fmt.value = 3;
	make_format_packet(my_ctlr, &data_buf, tmp_fmt);
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);

	make_trigger_packet(my_ctlr, &data_buf);
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);
	sleep(6);

	make_reset_packet(my_ctlr, &data_buf, address[0]);
	serial_put_buffer(&my_port, data_buf);
	serial_send(&my_port);
	reset_data_buf(&data_buf);
	sleep(2);

	/* stop requested routine */
	for (uint8_t i = 0; i < 3; i++) {
		make_text(my_ctlr, &data_buf,
			address[0], "STOP REQUESTED");
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);

		tmp_fmt.name = 'R';
		tmp_fmt.value = 50;
		make_format_packet(my_ctlr, &data_buf, tmp_fmt);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);

		make_trigger_packet(my_ctlr, &data_buf);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);
		sleep(8);

		make_text(my_ctlr, &data_buf,
			address[0], "PLEASE USE REAR EXIT");
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);

		tmp_fmt.name = 'S';
		tmp_fmt.value = 3;
		make_format_packet(my_ctlr, &data_buf, tmp_fmt);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);

		make_trigger_packet(my_ctlr, &data_buf);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);
		sleep(8);
	}
#endif

exit:
	serial_close_port(&my_port);

	return 0;
}
