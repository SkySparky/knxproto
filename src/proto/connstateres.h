/* KNX Client Library
 * A library which provides the means to communicate with several
 * KNX-related devices or services.
 *
 * Copyright (C) 2014-2015, Ole Krüger <ole@vprsm.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KNXPROTO_PROTO_CONNSTATERES_H_
#define KNXPROTO_PROTO_CONNSTATERES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Connection State Response
 */
typedef struct {
	/**
	 * Communication channel
	 */
	uint8_t channel;

	/**
	 * Connection state (`0` is good)
	 */
	uint8_t status;
} knx_connection_state_response;

/**
 * Generate a raw connection response.
 *
 * \see KNX_CONNECTION_STATE_RESPONSE_SIZE
 * \param buffer Output buffer, you have to make sure there is enough space
 * \param res    Input connection state response
 */
void knx_connection_state_response_generate(
	uint8_t*                             buffer,
	const knx_connection_state_response* res
);

/**
 * Parse a raw connection response.
 *
 * \param message        Raw connection response
 * \param message_length Number of bytes in `message`
 * \param res            Output connection state response
 */
bool knx_connection_state_response_parse(
	const uint8_t*                 message,
	size_t                         message_length,
	knx_connection_state_response* res
);

/**
 * Connection state response size
 */
#define KNX_CONNECTION_STATE_RESPONSE_SIZE 2

#endif
