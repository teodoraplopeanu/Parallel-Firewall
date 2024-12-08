// SPDX-License-Identifier: BSD-3-Clause
#include <stdlib.h>
#include "ring_buffer.h"
#include "utils.h"

#define PKT_SZ 256

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	ring->cap = cap;
	ring->data = malloc(cap);
	DIE(!ring->data, "Malloc failed!");
	ring->write_pos = 0;
	ring->read_pos = 0;
	ring->len = 0;
	ring->stop = false;

	// Synchronization primitives
	pthread_mutex_init(&ring->mutex, NULL);

	sem_init(&ring->sem_empty, 0, ring->cap / PKT_SZ);
	sem_init(&ring->sem_full, 0, 0);

	return 0;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	if (!ring)
		return -1;

	sem_wait(&ring->sem_empty);

	pthread_mutex_lock(&ring->mutex);
	memcpy(ring->data + ring->write_pos, data, size);
	ring->write_pos = (ring->write_pos + size) % ring->cap;
	ring->len += size;
	pthread_mutex_unlock(&ring->mutex);

	sem_post(&ring->sem_full);

	return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	if (!ring)
		return -1;

	sem_wait(&ring->sem_full);

	pthread_mutex_lock(&ring->mutex);
	memcpy(data, ring->data + ring->read_pos, size);
	ring->read_pos = (ring->read_pos + size) % ring->cap;
	ring->len -= size;
	pthread_mutex_unlock(&ring->mutex);

	sem_post(&ring->sem_empty);

	return size;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	free(ring->data);
	ring->data = NULL;
	ring->cap = 0;
	ring->len = 0;
	ring->write_pos = 0;
	ring->read_pos = 0;

	pthread_mutex_destroy(&ring->mutex);

	sem_destroy(&ring->sem_empty);
	sem_destroy(&ring->sem_full);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	ring->stop = true;
}
