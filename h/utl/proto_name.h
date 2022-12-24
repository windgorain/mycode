/* Copyright (C) 2007-2010 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Gurvinder Singh <gurvindersinghdahiya@gmail.com>
 */

#ifndef __PROTO_NAME_H__
#define	__PROTO_NAME_H__

#include <stdint.h>

#ifndef OS_WIN32
#define PROTO_FILE    "/etc/protocols"
#else
#define PROTO_FILE    "C:\\Windows\\system32\\drivers\\etc\\protocol"
#endif /* OS_WIN32 */

/** Lookup array to hold the information related to known protocol
 *  in /etc/protocols */
//extern char *known_proto[256];

uint8_t ProtoNameValid(uint16_t);
void ProtoNameInit(char *p[256]);
void ProtoNameDeInit(char *p[256]);

#endif	/* __UTIL_PROTO_NAME_H__ */

