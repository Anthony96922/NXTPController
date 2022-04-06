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
		"Usage: %s\n"
		"\t[ -p port ] [ -a address|addr1,addr2 ]\n"
		"\t-t text [ -f fmt-name,fmt-value ... ]\n"
		"\n"
		"\t-p port\t\t\tUART port to use (default: \"%s\")\n"
		"\t-a address\t\tAddress of one or more signs\n"
		"\t-t text\t\t\tText string to use\n"
		"\t-f name,value\t\tFormat name and value\n"
		"\t-c mid,extPid,pid\tController configuration\n"
		"\n"
		"\t-r\t\t\tRailroad X-ing mode\n"
		"\n"
		"\t-h\t\t\tShow this help and exit\n"
		"\t-v\t\t\tShow version and exit\n"
		"\n",
	name, DEFAULT_PORT);
}

int main(int argc, char *argv[]) {
	int opt;
	char text[MAX_TEXT_LEN + 1] = {0};
	struct data_buf_t data_buf;
	char port[PORT_SIZE] = {0};
	struct serialport_t my_port;
	struct ctlr_cfg_t my_ctlr;
	uint8_t address[2] = {0}; /* default to all signs */
	struct text_fmt_t fmt[5];
	uint8_t fmt_idx = 0;
	uint8_t rrxing = 0;

	const char *short_opt = "p:a:t:c:rhv";
	const struct option long_opt[] = {
		{"port",	required_argument,	NULL,	'p'},
		{"address",	required_argument,	NULL,	'a'},
		{"text",	required_argument,	NULL,	't'},
		{"format",	required_argument,	NULL,	'f'},
		{"ctlr",	required_argument,	NULL,	'c'},

		/* preset functions */
		{"rrxing", 	no_argument,		NULL,	'r'},

		{"help",	no_argument,		NULL,	'h'},
		{"version",	no_argument,		NULL,	'v'},
		{0,		0,			0,	0}
	};

	/* default sign controller configuration */
	set_ctlr_config(&my_ctlr, 195, 255, 245);

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
	{
		switch (opt) {
			case 'p':
				strncpy(port, optarg, PORT_SIZE - 1);
				printf("Using serial port \"%s\".\n", port);
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

			case 'c':
				/* sign controller configuration */
				if (sscanf(optarg, "%hhu,%hhu,%hhu",
					&my_ctlr.mid,
					&my_ctlr.ext_pid,
					&my_ctlr.pid) == 3) {
					printf("Using controller configuration"
						" %u/%u/%u.\n",
						my_ctlr.mid,
						my_ctlr.ext_pid,
						my_ctlr.pid);
				} else {
					printf("Invalid controller config"
						" syntax.\n");
				}
				break;

			case 'r':
				if (address[0] && address[1]) {
					rrxing++;
					printf("Activated railroad"
						" crossing mode.\n");
				} else {
					printf("Please specify 2 sign"
						" addresses to enable"
						" RR x-ing mode.\n");
					return 1;
				}
				break;

			case 'v':
				printf(VERSION "\n");
				return 0;

			case 'h':
			default:
				show_help(argv[0]);
				return 1;
		}
	}

	if (!port[0]) {
		strcpy(port, DEFAULT_PORT);
		printf("Using default port \"%s\".\n", port);
	}

	/* open the serial port (9600 8n1) */
	if (serial_open_port(&my_port, port) < 0) return 1;

	if (rrxing) {
		/* reset the signs to get them ready */
		for (uint8_t j = 0; j < 2; j++) {
			make_reset_packet(my_ctlr, &data_buf, address[j]);
			serial_put_buffer(&my_port, data_buf);
			serial_send(&my_port);
			reset_data_buf(&data_buf);

			make_text(my_ctlr, &data_buf, address[j],
					j ?
					/* for alternating between halves */
					"^Y12^W5^Y13^W5^R" :
					"^Y13^W5^Y12^W5^R"
			);
			serial_put_buffer(&my_port, data_buf);

			make_trigger_packet(my_ctlr, &data_buf);
			serial_put_buffer(&my_port, data_buf);
			serial_send(&my_port);
			reset_data_buf(&data_buf);
		}

		goto exit;
	}

	if (text[0]) {
		/* reset the sign */
		make_reset_packet(my_ctlr, &data_buf, address[0]);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);

		/* send text packets */
		make_text(my_ctlr, &data_buf,
			address[0], text);
		serial_put_buffer(&my_port, data_buf);

		/* send optional format packets */
		for (uint8_t i = 0; i < fmt_idx; i++) {
			make_format_packet(my_ctlr, &data_buf, fmt[i]);
			serial_put_buffer(&my_port, data_buf);
		}

		/* send trigger packet */
		make_trigger_packet(my_ctlr, &data_buf);
		serial_put_buffer(&my_port, data_buf);
		serial_send(&my_port);
		reset_data_buf(&data_buf);
	}

exit:
	serial_close_port(&my_port);

	return 0;
}
