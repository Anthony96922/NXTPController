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

/* get the number of segments needed to transmit a message */
static uint8_t get_num_segs(uint8_t len) {
	uint16_t i;
	uint8_t k = 0;
	uint8_t segments = 0;

	for (i = 0; i < len; i++) {
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
	if (len % 12) segments++;

#ifdef DEBUG
	printf("(%s): length: %u, segments needed: %u\n",
		__func__, len, segments);
#endif

	return segments;
}

static uint16_t make_text_pkts(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t address, char *text) {
	char segment[MAX_TEXT_SEG_LEN + 1];
	uint8_t num_segs;
	uint16_t buf_len = 0;

	num_segs = get_num_segs(strlen(text));

	/* create as many M packets as needed for the entire string */
	for (uint8_t i = 0; i < num_segs; i++) {
		memset(segment, 0, MAX_TEXT_SEG_LEN + 1);
		strncpy(segment, text + MAX_TEXT_SEG_LEN * i,
			MAX_TEXT_SEG_LEN);
		buf_len += make_m_pkt(buf + buf_len,
					ctlr,
					address,
					1,
					(((1 + i) & 15) << 4) | 1,
					segment);
	}

	return buf_len;
}

/*
 * static text (sign will scroll text if longer than 16 chars)
 *
 */
void static_text(struct ctlr_cfg_t ctlr, uint8_t address,
	char *text, uint8_t seconds, char *out_buf, uint16_t *buf_len) {

	/* create one or more M packets */
	*buf_len = make_text_pkts(out_buf, ctlr, address, text);

	/* create the F packet */
	*buf_len += make_f_pkt(out_buf + *buf_len, ctlr, 'R', seconds * 10);

	/* create the T packet */
	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}

/*
 * scrolling text
 *
 */
void scrolling_text(struct ctlr_cfg_t ctlr, uint8_t address,
	char *text, uint8_t speed, char *out_buf, uint16_t *buf_len) {

	/* create one or more M packets */
	*buf_len = make_text_pkts(out_buf, ctlr, address, text);

	/* create the F packet */
	*buf_len += make_f_pkt(out_buf + *buf_len, ctlr, 'S', speed);

	/* create the T packet */
	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}

/*
 * reset the sign immediately
 * useful for preempting important messages like next stop
 *
 */
void reset_sign(struct ctlr_cfg_t ctlr,
	uint8_t address, char *out_buf, uint8_t *buf_len) {

	*buf_len = 0;

	/* make a single M packet to reset the sign */
	*buf_len += make_m_pkt(out_buf + *buf_len,
				ctlr,
				address,
				1,
				0, /* position = 0: reset the sign */
				"0");

	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}
