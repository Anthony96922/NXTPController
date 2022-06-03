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

#define MAX_ADDRESSES	10
#define MAX_FORMAT_OPTS	10

typedef struct signctl_obj_t {
	uint8_t address;
	struct ctlr_cfg_t *ctlr;
	struct data_buf_t *data;
	struct serialport_t *port;
} signctl_obj_t;

/* needed to work around implicit declaration */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/* millisecond sleep */
static void msleep(unsigned int ms) {
	struct timespec ts;
	ts.tv_sec = ms / 1000u;			/* whole seconds */
	ts.tv_nsec = (ms % 1000u) * 1000000;	/* remainder, in nanoseconds */
	nanosleep(&ts, NULL);
}

static void show_help(char *name) {
	fprintf(stderr,
		"Sunrise Systems NXTP Sign Controller v" VERSION "\n"
		"\n"
		"Usage: %s -t text [ -p port ] [ -a address ... ]\n"
		"\t[ -f fmt-name,fmt-value ... ] [ -c mid,extPid,pid ]\n"
		"\n"
		"\t-p port\t\t\tUART port to use (default: \"%s\")\n"
		"\t-a address\t\tAddress of one or more signs\n"
		"\t-t text\t\t\tText string to use\n"
		"\t-f name,value\t\tOne or more format name and value pairs\n"
		"\t-c mid,extPid,pid\tJ1587 controller configuration\n"
		"\t-r\t\t\tReset signs before new sending new data\n"
		"\t-l\t\t\tClock mode"
		"\n"
		"\t-h\t\t\tShow this help and exit\n"
		"\t-v\t\t\tShow version and exit\n"
		"\n",
	name, DEFAULT_PORT);
}

static uint8_t shutdown;

static void *clock_worker(void *arg) {
	char text[14];
	struct tm utc;
	time_t now;
	int8_t minutes = -1;

	struct signctl_obj_t *local_obj = (struct signctl_obj_t *)arg;
	struct ctlr_cfg_t local_ctlr = *local_obj->ctlr;
	struct data_buf_t local_data_buf = *local_obj->data;
	struct serialport_t local_port = *local_obj->port;

	memset(&utc, 0, sizeof(struct tm));

	while (!shutdown) {
		/* check time */
		now = time(NULL);
		memcpy(&utc, gmtime(&now), sizeof(struct tm));

		if (utc.tm_min != minutes) {
			/* send text packets */
			sprintf(text, "%02u:%02u UTC",
				utc.tm_hour, utc.tm_min);
			make_text(local_ctlr, &local_data_buf,
				local_obj->address, text);
			serial_put_buffer(&local_port, local_data_buf);
			make_trigger_packet(local_ctlr, &local_data_buf);
			serial_put_buffer(&local_port, local_data_buf);
			serial_send(&local_port);
			reset_data_buf(&local_data_buf);
			/* update minute counter */
			minutes = utc.tm_min;
		}
		msleep(100);
	}

	pthread_exit(NULL);
}

static void exit_clock() {
	shutdown = 1;
}

int main(int argc, char *argv[]) {
	int opt;
	char text[MAX_TEXT_LEN + 1] = {0};
	char port[PORT_SIZE] = {0};

	/* serial data buffer */
	struct data_buf_t data_buf;

	struct serialport_t my_port;

	/* sign controller configuration */
	struct ctlr_cfg_t my_ctlr;

	/* store multiple sign addresses */
	uint8_t address[MAX_ADDRESSES] = {0}; /* default to all signs */
	uint8_t addr_idx = 0;

	/* store multiple format options */
	struct text_fmt_t fmt[MAX_FORMAT_OPTS];
	uint8_t fmt_idx = 0;

	/* reset signs if desired */
	uint8_t reset = 0;

	uint8_t clock_mode = 0;
	pthread_t clock_thread;
	pthread_attr_t attr;

	struct signctl_obj_t clock_obj;

	signal(SIGINT, exit_clock);
	signal(SIGTERM, exit_clock);

	const char *short_opt = "p:a:t:f:c:lrhv";
	const struct option long_opt[] = {
		{"port",	required_argument,	NULL,	'p'},
		{"address",	required_argument,	NULL,	'a'},
		{"text",	required_argument,	NULL,	't'},
		{"format",	required_argument,	NULL,	'f'},
		{"ctlr",	required_argument,	NULL,	'c'},
		{"clock",	no_argument,		NULL,	'l'},
		{"reset",	no_argument,		NULL,	'r'},

		/* preset functions */
		/* (none) */

		{"help",	no_argument,		NULL,	'h'},
		{"version",	no_argument,		NULL,	'v'},
		{0,		0,			0,	0}
	};

	/* default sign controller configuration */
	set_ctlr_config(&my_ctlr, 195, 255, 245);

keep_parsing_opts:

	opt = getopt_long(argc, argv, short_opt, long_opt, NULL);
	if (opt == -1) goto done_parsing_opts;

	switch (opt) {
		case 'p':
			strncpy(port, optarg, PORT_SIZE - 1);
			printf("Using serial port \"%s\".\n", port);
			break;

		case 'a':
			if (addr_idx < MAX_ADDRESSES) {
				address[addr_idx] =
					strtoul(optarg, NULL, 10);
				printf("Using sign address %u.\n",
					address[addr_idx++]);
			} else {
				fprintf(stderr,
					"Too many addresses.\n");
				return 1;
			}
			break;

		case 't':
			strncpy(text, optarg, MAX_TEXT_LEN);
			printf("Using text \"%s\".\n", text);
			break;

		case 'f':
			/* text format options */
			if (fmt_idx < MAX_FORMAT_OPTS) {
				if (sscanf(optarg, "%c,%hhu",
					&fmt[fmt_idx].name,
					&fmt[fmt_idx].value) == 2) {
					printf("Using format '%c' = %u.\n",
						fmt[fmt_idx].name,
						fmt[fmt_idx].value);
					fmt_idx++;
				} else if (sscanf(optarg, "%c,%c",
					&fmt[fmt_idx].name,
					&fmt[fmt_idx].value) == 2) {
					printf("Using format '%c' = '%c'.\n",
						fmt[fmt_idx].name,
						fmt[fmt_idx].value);
					fmt_idx++;
				} else {
					printf("Invalid format syntax.\n");
				}
			} else {
				fprintf(stderr, "Too many format options.\n");
				return 1;
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
					my_ctlr.mid, my_ctlr.ext_pid,
					my_ctlr.pid);
			} else {
				fprintf(stderr,
					"Invalid controller config syntax.\n");
				return 1;
			}
			break;

		case 'l':
			printf("Enabling clock mode.\n");
			clock_mode = 1;
			break;

		case 'r':
			reset = 1;
			break;

		case 'v':
			printf("version " VERSION "\n");
			return 0;

		case 'h':
		default:
			show_help(argv[0]);
			return 1;
	}

	goto keep_parsing_opts;

done_parsing_opts:

	if (!text[0] && !clock_mode) {
		fprintf(stderr, "No text specified.\n\n");
		show_help(argv[0]);
		return 1;
	}

	if (!port[0]) {
		strcpy(port, DEFAULT_PORT);
		printf("Using default port \"%s\".\n", port);
	}

	if (!addr_idx) {
		printf("Broadcasting to all signs.\n");
		addr_idx = 1;
	}

	/* open the serial port (9600 8n1) */
	if (serial_open_port(&my_port, port) < 0) return 1;

	if (clock_mode) {
		clock_obj.address = address[0];
		clock_obj.ctlr = &my_ctlr;
		clock_obj.data = &data_buf;
		clock_obj.port = &my_port;

		pthread_attr_init(&attr);
		if (pthread_create(&clock_thread, &attr, clock_worker, (void *)&clock_obj) < 0)
			fprintf(stderr, "Could not start thread.\n");

		pthread_attr_destroy(&attr);

		while (1) {
			if (shutdown) break;
			msleep(100);
		}

		pthread_join(clock_thread, NULL);
	} else {
		for (uint8_t i = 0; i < addr_idx; i++) {
			/* reset the sign */
			if (reset) {
				make_reset_packet(my_ctlr, &data_buf,
					address[i]);
				serial_put_buffer(&my_port, data_buf);
				serial_send(&my_port);
				reset_data_buf(&data_buf);
			}

			/* send text packets */
			make_text(my_ctlr, &data_buf,
				address[i], text);
			serial_put_buffer(&my_port, data_buf);

			/* send optional format packets */
			for (uint8_t j = 0; j < fmt_idx; j++) {
				make_format_packet(my_ctlr, &data_buf, fmt[j]);
				serial_put_buffer(&my_port, data_buf);
			}

			/* send trigger packet */
			make_trigger_packet(my_ctlr, &data_buf);
			serial_put_buffer(&my_port, data_buf);
			serial_send(&my_port);
			reset_data_buf(&data_buf);
		}
	}

	serial_close_port(&my_port);

	return 0;
}
