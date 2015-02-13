#ifndef HPKNXD_KNX_TUNNELREQ_H
#define HPKNXD_KNX_TUNNELREQ_H

#include "../msgbuilder.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Tunnel Request
 */
typedef struct {
	uint8_t channel;
	uint8_t seq_number;
	uint16_t size;
	const void* data;
} knxnetip_tunnel_request;

/**
 * Generate the message for a tunnel request.
 */
bool knxnetip_append_tunnel_request(msgbuilder* mb,
                                    const knxnetip_tunnel_request* req);

/**
 * Parse a message (excluding header) which contains a tunnel request.
 */
bool knxnetip_parse_tunnel_request(const uint8_t* message, size_t length,
                                   knxnetip_tunnel_request* req);

#endif