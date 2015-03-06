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

#include "routerclient.h"

#include "proto/knxnetip.h"

#include "util/log.h"
#include "util/alloc.h"
#include "util/sockutils.h"

#include <sys/socket.h>
#include <netinet/in.h>

bool knx_router_connect(knx_router_client* client, const ip4addr* router) {
	if (router) {
		client->router = *router;
	} else {
		ip4addr_from_string(&client->router, "224.0.23.12", 3671);
	}

	ip4addr local = client->router;
	local.sin_addr.s_addr = INADDR_ANY;

	if ((client->sock = dgramsock_create(&local, true)) < 0) {
		log_error("Failed to create socket");
		return false;
	}

	// Join multicast group
	client->mreq.imr_interface.s_addr = INADDR_ANY;
	client->mreq.imr_multiaddr.s_addr = client->router.sin_addr.s_addr;

	if (setsockopt(client->sock, IPPROTO_IP,
	               IP_ADD_MEMBERSHIP, &client->mreq, sizeof(client->mreq)) < 0) {
		log_error("Could not join multicast group");
		return false;
	}

	// Turn off multicast loopback
	setsockopt(client->sock, IPPROTO_IP, IP_MULTICAST_LOOP, anona(int, 0), sizeof(int));

	return true;
}

bool knx_router_disconnect(const knx_router_client* client) {
	bool r = setsockopt(client->sock, IPPROTO_IP,
	                    IP_DROP_MEMBERSHIP, &client->mreq, sizeof(client->mreq)) == 0;
	close(client->sock);
	return r;
}

static ssize_t knx_router_recv_raw(const knx_router_client* client, uint8_t** result_buffer, bool block) {
	if (!block && !dgramsock_ready(client->sock, 0, 0))
		return -1;

	ssize_t buffer_size = dgramsock_peek_knx(client->sock);

	if (buffer_size < 0) {
		// Discard this packet
		recvfrom(client->sock, NULL, 0, 0, NULL, NULL);

		log_warn("Dequeued bogus message");

		// We have to rely on the compiler to perform tail-call optimization here,
		// otherwise this might turn out horribly.
		// Alternatively we could use a goto ...
		return knx_router_recv_raw(client, result_buffer, block);
	}

	uint8_t buffer[buffer_size];
	knx_packet packet;

	if (dgramsock_recv_knx(client->sock, buffer, buffer_size, &packet, NULL, 0) &&
	    packet.service == KNX_ROUTING_INDICATION) {
		uint8_t* payload = newa(uint8_t, packet.payload.routing_ind.size);

		if (!payload)
			return -1;

		memcpy(payload, packet.payload.routing_ind.data, packet.payload.routing_ind.size);

		*result_buffer = payload;
		return packet.payload.routing_ind.size;
	} else
		return -1;
}

knx_ldata* knx_router_recv(const knx_router_client* client, bool block) {
	uint8_t* data;
	ssize_t size = knx_router_recv_raw(client, &data, block);

	knx_cemi_frame cemi;

	if (size < 0)
		return NULL;

	if (!knx_cemi_parse(data, size, &cemi) || (cemi.service != KNX_CEMI_LDATA_IND &&
	                                           cemi.service != KNX_CEMI_LDATA_CON)) {
		log_error("Invalid frame contents");
		free(data);

		// Rely on tail-call optimization, otherwise this will get messy
		return knx_router_recv(client, block);
	}

	switch (cemi.payload.ldata.tpdu.tpci) {
		case KNX_TPCI_UNNUMBERED_DATA:
		case KNX_TPCI_NUMBERED_DATA: {
			uint8_t safe[cemi.payload.ldata.tpdu.info.data.length];
			memcpy(safe, cemi.payload.ldata.tpdu.info.data.payload, sizeof(safe));

			knx_ldata* ret = realloc(data, sizeof(knx_ldata) + sizeof(safe));

			if (ret) {
				*ret = cemi.payload.ldata;
				ret->tpdu.info.data.payload = (const uint8_t*) (ret + 1);
				memcpy(ret + 1, safe, sizeof(safe));
			}

			return ret;
		}

		case KNX_TPCI_UNNUMBERED_CONTROL:
		case KNX_TPCI_NUMBERED_CONTROL: {
			knx_ldata* ret = realloc(data, sizeof(knx_ldata));
			*ret = cemi.payload.ldata;
			return ret;
		}
	}
}

static bool knx_router_send_raw(const knx_router_client* client, const uint8_t* payload, uint16_t length) {
	knx_routing_indication route_ind = {
		length,
		payload
	};

	return dgramsock_send_knx(client->sock, KNX_ROUTING_INDICATION, &route_ind, &client->router);
}

bool knx_router_send(const knx_router_client* client, const knx_ldata* ldata) {
	uint8_t buffer[knx_cemi_size(KNX_CEMI_LDATA_IND, ldata)];
	knx_cemi_generate_(buffer, KNX_CEMI_LDATA_IND, ldata);
	return knx_router_send_raw(client, buffer, sizeof(buffer));
}

bool knx_router_write_group(knx_router_client* client, knx_addr dest,
                            knx_dpt type, const void* value) {
	uint8_t buffer[knx_dpt_size(type)];
	knx_dpt_to_apdu(buffer, type, value);

	knx_ldata frame = {
		.control1 = {KNX_LDATA_PRIO_LOW, true, true, false, false},
		.control2 = {KNX_LDATA_ADDR_GROUP, 7},
		.source = 0,
		.destination = dest,
		.tpdu = {
			.tpci = KNX_TPCI_UNNUMBERED_DATA,
			.info = {
				.data = {
					.apci = KNX_APCI_GROUPVALUEWRITE,
					.payload = buffer,
					.length = sizeof(buffer)
				}
			}
		}
	};

	return knx_router_send(client, &frame);
}
