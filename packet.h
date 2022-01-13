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

#define MAX_PKT_LEN		21
#define MAX_TEXT_SEG_LEN	12
#define MAX_TEXT_LEN		156

/* sign controller configuration */
typedef struct ctlr_cfg_t {
	uint8_t mid;
	uint8_t ext_pid;
	uint8_t pid;
} ctlr_cfg_t;

extern void set_ctlr_config(struct ctlr_cfg_t *ctlr_cfg,
	uint8_t mid, uint8_t ext_pid, uint8_t pid);

/*
 * J1587 packet structures
 * (checksum is added later)
 *
 */

/* M (message) */
typedef struct msg_m_t {
	uint8_t mid;
	uint8_t ext_pid;
	uint8_t pid;
	/* num of bytes following excl. checksum */
	uint8_t len;
	uint8_t pkt_type;	/* 'M' */
	/* sign address */
	uint8_t address;
	uint8_t line_num;	/* usually set to 1 */
	/* horizontal/vertical position (h << 4) | v */
	uint8_t position;
} msg_m_t;

#define MSG_M_SIZE	sizeof(struct msg_m_t)

/* F (format) */
typedef struct msg_f_t {
	uint8_t mid;
	uint8_t ext_pid;
	uint8_t pid;
	/* num of bytes following excl. checksum */
	uint8_t len;		/* 3 */
	uint8_t pkt_type;	/* 'F' */
	/* format parameter */
	uint8_t param;
	/* format value */
	uint8_t value;
} msg_f_t;

#define MSG_F_SIZE	sizeof(struct msg_f_t)

/* T (trigger) */
typedef struct msg_t_t {
	uint8_t mid;
	uint8_t ext_pid;
	uint8_t pid;
	/* num of bytes following excl. checksum */
	uint8_t len;		/* 1 */
	uint8_t pkt_type;	/* 'T' */
} msg_t_t;

#define MSG_T_SIZE	sizeof(struct msg_t_t)

/* request parameter (384) */
typedef struct msg_rp_t {
	uint8_t mid;
	uint8_t ext_pid;
	uint8_t pid;	/* 128 */
	uint8_t pid2;	/* 254 */
	uint8_t sign_mid;
} msg_rp_t;

#define MSG_RP_SIZE	sizeof(struct msg_rp_t)

/* data link escape (510) */
typedef struct msg_dle_t {
	uint8_t sign_mid;	/* 189 */
	uint8_t ext_pid;
	uint8_t pid;		/* 254 */
	uint8_t mid;
	uint8_t len;		/* 7 */
	uint8_t state;		/* 'R', 'B' or 'P' */
	uint8_t host_mid;
	uint8_t tbmu;
	uint8_t tbml;
	uint8_t fbm;
	uint8_t aux_state;	/* 'S' or 'F' */
} msg_dle_t;

#define MSG_DLE_SIZE	sizeof(struct msg_dle_t)

extern uint8_t make_m_pkt(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t address, uint8_t line_num, uint8_t position, char *text);
extern uint8_t make_f_pkt(char *buf, struct ctlr_cfg_t ctlr,
	uint8_t param, uint8_t value);
extern uint8_t make_t_pkt(char *buf, struct ctlr_cfg_t ctlr);

#ifdef DEBUG
extern void print_bytes(char *msg, uint8_t len);
#endif
