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

/* append the checksum at the end of the packet */
static void add_checksum(char *pkt, uint8_t pkt_len) {
	char csum = 0;

	for (uint8_t i = 0; i < pkt_len; i++) {
		csum += pkt[i];
	}

	/* two's complement */
	pkt[pkt_len] = ~csum + 1;
}

void set_ctlr_config(struct ctlr_cfg_t *ctlr_cfg,
	uint8_t mid, uint8_t ext_pid, uint8_t pid) {

	/* sign controller configuration */
	ctlr_cfg->mid		= mid;
	ctlr_cfg->ext_pid	= ext_pid;
	ctlr_cfg->pid		= pid;
}

/*
 * stuff for constructing compliant J1587 packets
 */

uint8_t make_m_pkt(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t address, uint8_t line_num, uint8_t position, char *text) {
	struct msg_m_t msg;
	uint8_t text_len = strlen(text);

	/* create the M packet */
	msg.mid		= ctlr.mid;
	msg.ext_pid	= ctlr.ext_pid;
	msg.pid		= ctlr.pid;
	msg.len		= 4 + text_len;
	msg.pkt_type	= 'M';
	msg.address	= address;
	msg.line_num	= line_num;
	msg.position	= position;

	/* copy to output buffer */
	memcpy(buf, &msg, MSG_M_SIZE);

	/* add text */
	memcpy(buf + MSG_M_SIZE, text, text_len);

	/* finally, add the checksum to complete the packet */
	add_checksum(buf, MSG_M_SIZE + text_len);

	return MSG_M_SIZE + text_len + 1;
}

uint8_t make_f_pkt(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t param, uint8_t value) {
	struct msg_f_t msg;

	/* create the F packet */
	msg.mid		= ctlr.mid;
	msg.ext_pid	= ctlr.ext_pid;
	msg.pid		= ctlr.pid;
	msg.len		= 3;
	msg.pkt_type	= 'F';
	msg.param	= param;
	msg.value	= value;

	/* copy to output buffer */
	memcpy(buf, &msg, MSG_F_SIZE);

	/* finally, add the checksum to complete the packet */
	add_checksum(buf, MSG_F_SIZE);

	return MSG_F_SIZE + 1;
}

uint8_t make_t_pkt(char *buf, struct ctlr_cfg_t ctlr) {
	struct msg_t_t msg;

	/* create the T packet */
	msg.mid		= ctlr.mid;
	msg.ext_pid	= ctlr.ext_pid;
	msg.pid		= ctlr.pid;
	msg.len		= 1;
	msg.pkt_type	= 'T';

	/* copy to output buffer */
	memcpy(buf, &msg, MSG_T_SIZE);

	/* finally, add the checksum to complete the packet */
	add_checksum(buf, MSG_T_SIZE);

	return MSG_T_SIZE + 1;
}

#ifdef DEBUG
void print_bytes(char *msg, uint8_t len) {
	printf("(%s): length: %u, data:", __func__, len);
	for (uint8_t i = 0; i < len; i++) {
		if (msg[i] >= 0x20 && msg[i] <= 0x7e) {
			printf(" '%c'", msg[i]);
		} else {
			printf(" 0x%02x", msg[i]);
		}
	}
	printf("\n");
}
#endif
