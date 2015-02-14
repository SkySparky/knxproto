#include "tunnelclient.h"

void knx_tunnel_worker(knx_tunnel_connection* conn) {
	while (conn->do_work) {
		// Check if we need to send a heartbeat
		if (conn->established && difftime(time(NULL), conn->last_heartbeat) >= 30) {
			knx_connection_state_request req = {conn->channel, 0, conn->host_info};
			if (knx_outqueue_push(&conn->outgoing, KNX_CONNECTIONSTATE_REQUEST, &req))
				conn->last_heartbeat = time(NULL);
		}

		// Sender loop
		while (conn->do_work) {
			knx_service service;
			uint8_t* buffer;
			ssize_t buffer_size = knx_outqueue_pop(&conn->outgoing, &buffer, &service);

			if (buffer_size < 0)
				break;

			dgramsock_send(conn->sock, buffer, buffer_size, &conn->gateway);
			free(buffer);

			printf("worker: Sent (service = 0x%04X)\n", service);
		}

		size_t receive_iter = 0;

		// Receiver loop
		while (conn->do_work && receive_iter++ < 100 && dgramsock_ready(conn->sock, 0, 100000)) {
			uint8_t buffer[100];

			// FIXME: Do not allow every endpoint
			ssize_t r = dgramsock_recv(conn->sock, buffer, 100, NULL, 0);

			knx_packet pkg_in;
			if (r > 0 && knx_parse(buffer, r, &pkg_in)) {
				printf("worker: Received (service = 0x%04X)\n", pkg_in.service);

				switch (pkg_in.service) {
					// Connection successfully establish
					case KNX_CONNECTION_RESPONSE:
						conn->established = (pkg_in.payload.conn_res.status == 0);


						if (conn->established) {
							conn->channel = pkg_in.payload.conn_res.channel;
							conn->host_info = pkg_in.payload.conn_res.host;

							printf("worker: Connected (channel = %i)\n", conn->channel);
						} else {
							printf("worker: Connection failed (code = %i)\n",
							       pkg_in.payload.conn_res.status);
						}

						break;

					// Heartbeat
					case KNX_CONNECTIONSTATE_RESPONSE:
						if (pkg_in.payload.conn_state_res.channel == conn->channel)
							conn->established &= (pkg_in.payload.conn_state_res.status == 0);

						printf("worker: Heartbeat response (channel = %i, status = %i)\n",
						       pkg_in.payload.conn_state_res.channel,
						       pkg_in.payload.conn_state_res.status);
						break;

					// Result of a disconnect request (duh)
					case KNX_DISCONNECT_RESPONSE:
						conn->established &= !(pkg_in.payload.dc_res.channel == conn->channel &&
						                       pkg_in.payload.dc_req.status == 0);

						printf("worker: Disconnect (channel = %i, status = %i)\n",
						       pkg_in.payload.dc_req.channel,
						       pkg_in.payload.dc_req.status);

						// Gently shut down this worker thread
						if (!conn->established)
							conn->do_work = false;

						break;

					// Tunnel Request
					case KNX_TUNNEL_REQUEST:
						// Send tunnel response if a connection is established
						if (conn->established) {
							knx_tunnel_response res = {
								conn->channel,
								pkg_in.payload.tunnel_req.seq_number,
								0
							};
							knx_outqueue_push(&conn->outgoing, KNX_TUNNEL_RESPONSE, &res);
						}

						knx_pkgqueue_enqueue(&conn->incoming, &pkg_in);

						break;

					// Everything else should be ignored
					default:
						break;
				}
			} else if (r < 0)
				break;
		}
	}

	// TODO: Clear outgoing queue once more.

	// Free buffers and queues
	knx_pkgqueue_destroy(&conn->incoming);
	knx_outqueue_destroy(&conn->outgoing);

	// The worker has to terminate the connection
	dgramsock_close(conn->sock);
}

extern void* knx_tunnel_worker_thread(void* conn) {
	knx_tunnel_worker(conn);
	pthread_exit(NULL);
}

bool knx_tunnel_connect(knx_tunnel_connection* conn, const ip4addr* gateway) {
	conn->gateway = *gateway;
	conn->established = false;
	conn->channel = 0;
	conn->last_heartbeat = 0;
	conn->do_work = true;

	// Setup UDP socket
	if ((conn->sock = dgramsock_create(NULL, false)) < 0)
		return false;

	// Start the worker thread
	if (pthread_create(&conn->worker_thread, NULL, &knx_tunnel_worker_thread, conn) != 0) {
		dgramsock_close(conn->sock);
		return false;
	}

	knx_pkgqueue_init(&conn->incoming);
	knx_outqueue_init(&conn->outgoing);

	// Queue a connection request
	knx_connection_request req = {
		KNX_CONNECTION_REQUEST_TUNNEL,
		KNX_LAYER_TUNNEL,
		KNX_HOST_INFO_NAT(KNX_PROTO_UDP),
		KNX_HOST_INFO_NAT(KNX_PROTO_UDP)
	};
	knx_outqueue_push(&conn->outgoing, KNX_CONNECTION_REQUEST, &req);

	return true;
}

void knx_tunnel_disconnect(knx_tunnel_connection* conn, bool wait_for_worker) {
	if (conn->established) {
		// Queue a disconnect request
		knx_disconnect_request req = {conn->channel, 0, conn->host_info};
		knx_outqueue_push(&conn->outgoing, KNX_DISCONNECT_REQUEST, &req);
	} else {
		// Shut down the worker thread
		conn->do_work = false;
	}

	if (wait_for_worker)
		pthread_join(conn->worker_thread, NULL);
	else
		pthread_detach(conn->worker_thread);
}