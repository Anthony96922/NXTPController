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

typedef struct text_fmt_t {
	uint8_t name;
	uint8_t value;
} text_fmt_t;

extern void make_text(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	uint8_t address, char *text);
extern void make_format_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	struct text_fmt_t fmt);
extern void make_reset_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf,
	uint8_t address);
extern void make_trigger_packet(struct ctlr_cfg_t ctlr, struct data_buf_t *buf);
