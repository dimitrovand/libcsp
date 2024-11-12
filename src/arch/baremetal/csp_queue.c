#include <csp/arch/csp_queue.h>
#include <string.h>
#include <stdlib.h>
#include <csp/csp.h>
#include "csp_critical.h"

csp_queue_handle_t csp_queue_create_static(int length, size_t item_size, char * buffer, csp_static_queue_t * queue) {
	if (queue != NULL && buffer != NULL) {
		queue->buffer = buffer;
		queue->size = length;
		queue->item_size = item_size;
		queue->items = 0;
		queue->in = 0;
		queue->out = 0;
	} else {
		queue = NULL;
	}

	return queue;
}

int csp_queue_enqueue(csp_queue_handle_t queue, const void * value, uint32_t timeout) {
	int ret = CSP_QUEUE_ERROR;

	csp_critical_enter();

	if (queue->items != queue->size) {
		memcpy((char *)queue->buffer + (queue->in * queue->item_size), value, queue->item_size);
		queue->items++;
		queue->in = (queue->in + 1) % queue->size;

		ret = CSP_QUEUE_OK;
	}

	csp_critical_exit();

	return ret;
}

int csp_queue_enqueue_isr(csp_queue_handle_t queue, const void * value, int * task_woken) {
	return csp_queue_enqueue(queue, value, 0);
}

int csp_queue_dequeue(csp_queue_handle_t queue, void * buf, uint32_t timeout) {
	int ret = CSP_QUEUE_ERROR;

	csp_critical_enter();

	if (queue->items != 0) {
		memcpy(buf, (char *)queue->buffer + (queue->out * queue->item_size), queue->item_size);
		queue->items--;
		queue->out = (queue->out + 1) % queue->size;
		ret = CSP_QUEUE_OK;
	}

	csp_critical_exit();

	return ret;
}

int csp_queue_dequeue_isr(csp_queue_handle_t queue, void * buf, int * task_woken) {
	return csp_queue_dequeue(queue, buf, 0);
}

int csp_queue_size(csp_queue_handle_t queue) {
	csp_critical_enter();
	int items = queue->items;
	csp_critical_exit();
	return items;
}

int csp_queue_size_isr(csp_queue_handle_t queue) {
	return csp_queue_size(queue);
}

int csp_queue_free(csp_queue_handle_t queue) {
	csp_critical_enter();
	int free = queue->size - queue->items;
	csp_critical_exit();
	return free;
}

void csp_queue_empty(csp_queue_handle_t queue) {
	csp_critical_enter();
	queue->items = 0;
	queue->in = 0;
	queue->out = 0;
	csp_critical_exit();
}
