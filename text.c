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
#include "text.h"

/* get the number of segments needed to transmit a message */
static uint8_t get_num_segs(uint8_t len) {
	uint8_t k = 0;
	uint8_t segments = 0;

	for (uint8_t i = 0; i < len; i++) {
		if (++k == MAX_TEXT_SEG_LEN) {
			segments++;
			k = 0;
		}
	}

	/*
	 * add an extra segment if length is not a multiple of 12
	 *
	 * needed to accomodate the last segment with < 12 chars
	 */
	if (len % MAX_TEXT_SEG_LEN) segments++;

#ifdef DEBUG
	printf("(%s): length: %u, segments needed: %u,"
		" last segment will have %u characters\n",
		__func__, len, segments,
		MAX_TEXT_SEG_LEN - (segments * MAX_TEXT_SEG_LEN - len));
#endif

	return segments;
}

static uint16_t make_text_pkts(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t address, char *text) {
	char segment[MAX_TEXT_SEG_LEN + 1];
	uint8_t num_segs = get_num_segs(strlen(text));
	uint16_t buf_len = 0;
	uint8_t pkt_len;

	/* create as many M packets as needed for the entire string */
	for (uint8_t i = 0; i < num_segs; i++) {
		memset(segment, 0, MAX_TEXT_SEG_LEN + 1);
		strncpy(segment, text + MAX_TEXT_SEG_LEN * i,
			MAX_TEXT_SEG_LEN);
		pkt_len = make_m_pkt(buf + buf_len,
					ctlr,
					address,
					1,
					(((1 + i) & 15) << 4) | 1,
					segment);

#ifdef DEBUG
		print_bytes(buf + buf_len, pkt_len);
#endif
		buf_len += pkt_len;

	}

	return buf_len;
}

/*
 * display text (sign will scroll text if longer than 16 chars)
 *
 */
void make_text(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	uint8_t address, char *text) {

	/* create one or more M packets */
	buf->len = make_text_pkts(buf->data, ctlr, address, text);
}

/*
 * text formatting
 *
 */
void make_format_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	struct text_fmt_t fmt) {

	/* create the F packet */
	buf->len = make_f_pkt(buf->data, ctlr,
				fmt.name, fmt.value);

#ifdef DEBUG
	print_bytes(buf->data, buf->len);
#endif
}

/*
 * reset the sign immediately
 * useful for preempting important messages like next stop
 *
 */
void make_reset_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	uint8_t address) {

	/* make a single M packet to reset the sign */
	buf->len = make_m_pkt(buf->data,
				ctlr,
				address,
				1,
				0, /* position = 0: reset the sign */
				" ");

#ifdef DEBUG
	print_bytes(buf->data, buf->len);
#endif
}

/*
 * trigger packet
 *
 */
void make_trigger_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf) {
	buf->len = make_t_pkt(buf->data, ctlr);

#ifdef DEBUG
	print_bytes(buf->data, buf->len);
#endif
}
