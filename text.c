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
static uint8_t get_num_text_segments(char *text) {
	uint8_t i = 0, k = 0;
	uint8_t segments = 0;

	for (i = 0; i < 156+1; i++) {
		if (text[i] == '\0') break; /* end of string */

		k++;
		if (k == MAX_TEXT_LEN) {
			segments++;
			k = 0;
		}
	}

	segments += 1; /* last segment with < 12 chars */

	return segments;
}

void static_text(struct ctlr_cfg_t ctlr,
	char *text, uint8_t seconds, char *out_buf, uint8_t *buf_len) {
	char text_segment[MAX_TEXT_LEN+1+1];
	uint8_t text_len;
	*buf_len = 0;

	memset(&text_segment, 0, MAX_TEXT_LEN+1+1);
	strncpy(text_segment, text, MAX_TEXT_LEN+1);
	text_len = strlen(text);

	if (text_len > 12) { /* max text length for a single packet */
		/* only create two M packets */
		for (uint8_t i = 0; i < 2; i++) {
			memset(&text_segment, 0, 12+1);
			strncpy(text_segment,
				text+MAX_TEXT_LEN*i, MAX_TEXT_LEN);
			*buf_len += make_m_pkt(
						out_buf + *buf_len,
						ctlr,
						0,
						1,
						((1+i) << 4) | 1,
						text_segment);
		}
	} else {
		/* create one M packet */
		*buf_len += make_m_pkt(
					out_buf + *buf_len,
					ctlr,
					0,
					1,
					(1 << 4) | 1,
					text_segment);
	}

	/* create the F packet */
	*buf_len += make_f_pkt(out_buf + *buf_len, ctlr, 'R', seconds * 10);

	/* create the T packet */
	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}

void scrolling_text(struct ctlr_cfg_t ctlr,
	char *text, char *out_buf, uint8_t *buf_len) {
	char text_buf[156+1];
	char text_segment[MAX_TEXT_LEN+1];
	uint8_t text_len;
	*buf_len = 0;

	/* copy text to to internal buffer */
	memset(&text_buf, 0, 156+1);
	strncpy(text_buf, text, 156+1);
	text_len = strlen(text_buf);

	if (text_len > MAX_TEXT_LEN) { /* max text length for one packet */
		/* create as many M packets as needed for the message */
		for (uint8_t i = 0; i < get_num_text_segments(text_buf); i++) {
			memset(&text_segment, 0, 12+1);
			strncpy(text_segment,
				text_buf+MAX_TEXT_LEN*i, MAX_TEXT_LEN);
			*buf_len += make_m_pkt(
						out_buf + *buf_len,
						ctlr,
						0,
						1,
						((1+i) << 4) | 1,
						text_segment);
		}
	} else {
		*buf_len += make_m_pkt(
					out_buf + *buf_len,
					ctlr,
					0,
					1,
					(1 << 4) | 1,
					text_buf);
	}

	/* create the F packet */
	*buf_len += make_f_pkt(out_buf + *buf_len, ctlr, 'S', 5);

	/* create the T packet */
	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}

/*
 * reset the sign immediately
 * useful for preempting important messages like next stop
 */
void reset_sign(struct ctlr_cfg_t ctlr,
	char *out_buf, uint8_t *buf_len) {
	*buf_len = 0;

	*buf_len += make_m_pkt(out_buf + *buf_len,
				ctlr,
				0,
				1,
				0, /* position = 0: reset the sign */
				"0");

	*buf_len += make_t_pkt(out_buf + *buf_len, ctlr);

#ifdef DEBUG
	print_bytes(out_buf, *buf_len);
#endif
}
