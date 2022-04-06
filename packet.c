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
	uint8_t pkt_len = 0;

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
	pkt_len += MSG_M_SIZE;

	/* add text */
	memcpy(buf + pkt_len, text, text_len);
	pkt_len += text_len;

	/* finally, add the checksum to complete the packet */
	add_checksum(buf, pkt_len);
	pkt_len += 1;

	/* size check */
	if (pkt_len > MAX_PKT_LEN) pkt_len = MAX_PKT_LEN;

	return pkt_len;
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

/*
 * make RP (request parameter) packet
 *
 */
uint8_t make_rp_pkt(char *buf, struct ctlr_cfg_t ctlr) {
	struct msg_rp_t msg;

	/* create the RP packet */
	msg.mid		= ctlr.pid;
	msg.ext_pid	= ctlr.ext_pid;
	msg.pid		= 384 % 256;	/* 128 */
	msg.pid2	= 510 % 256;	/* 254 */
	msg.sign_mid	= 189;

	/* copy to output buffer */
	memcpy(buf, &msg, MSG_RP_SIZE);

	/* finally, add the checksum to complete the packet */
	add_checksum(buf, MSG_RP_SIZE);

	return MSG_RP_SIZE + 1;
}

/*
 * RP response packet
 *
 */
void read_dle_pkt(char *buf, uint8_t len, struct msg_dle_t *msg) {
	if (len >= MSG_DLE_SIZE) {
		/* copy received packet as is */
		memcpy(msg, buf, MSG_DLE_SIZE);
	}

#ifdef DEBUG
	printf("(%s): %u %u %u"
		" %u %u %u"
		" '%c' %u %02x %02x %02x '%c' %02x\n",
		__func__,
		msg->sign_mid, msg->ext_pid, msg->pid, msg->mid,
		msg->len,
		msg->address,
		msg->state,
		msg->host_mid,
		msg->tbmu, msg->tbml, msg->fbm,
		msg->aux_state,
		msg->checksum
	);
#endif
}

/*
 * reset the data buffer
 *
 */
void reset_data_buf(struct data_buf_t *data_buf) {
	memset(data_buf->data, 0, BUF_LEN);
	data_buf->len = 0;
}

#ifdef DEBUG
/*
 * output raw data
 *
 */
void print_bytes(char *msg, uint16_t len) {
	printf("(%s): length: %u, data:", __func__, len);
	for (uint16_t i = 0; i < len; i++) {
		if (msg[i] >= 0x20 && msg[i] <= 0x7e) {
			printf(" '%c'", msg[i]);
		} else {
			printf(" %02x", msg[i]);
		}
	}
	printf("\n");
}
#endif
