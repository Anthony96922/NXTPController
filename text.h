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

extern void static_text(struct ctlr_cfg_t ctlr,
	char *text, uint8_t seconds, char *out_buf, uint8_t *buf_len);
extern void scrolling_text(struct ctlr_cfg_t ctlr,
	char *text, char *out_buf, uint8_t *buf_len);
extern void reset_sign(struct ctlr_cfg_t ctlr,
 	char *out_buf, uint8_t *buf_len);
