// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void *consumer_thread(void *ctx_void)
{
	so_consumer_ctx_t *ctx = (so_consumer_ctx_t *)ctx_void;
	char buff[PKT_SZ];
	char out_buf[PKT_SZ];

	while (true) {
		if (ctx->producer_rb->stop == true && ctx->producer_rb->len == 0)
			break;

		ssize_t bytes_read = ring_buffer_dequeue(ctx->producer_rb, buff, PKT_SZ);

		if (bytes_read > 0) {
			so_packet_t *pkt = (so_packet_t *)buff;
			int action = process_packet(pkt);
			unsigned long hash = packet_hash(pkt);
			unsigned long timestamp = pkt->hdr.timestamp;

			int len = snprintf(out_buf, 256, "%s %016lx %lu\n",
				RES_TO_STR(action), hash, timestamp);

			write(ctx->out_fd, out_buf, len);
		}
	}

	return NULL;
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					int out_fd)
{
	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));
		DIE(!ctx, "Malloc failed!");

		ctx->producer_rb = rb;
		ctx->out_fd = out_fd;

		pthread_create(&tids[i], NULL, &consumer_thread, (void *)ctx);
	}

	return num_consumers;
}
