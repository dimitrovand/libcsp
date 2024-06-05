#include <csp/csp_sfp.h>

#include <stdlib.h>

#include <csp/csp_buffer.h>
#include <csp/csp_debug.h>
#include "csp_macro.h"
#include <endian.h>

#include "csp_conn.h"

typedef struct __packed {
	uint32_t offset;
	uint32_t totalsize;
} sfp_header_t;

/**
 * SFP Headers:
 * The following functions are helper functions that handles the extra SFP
 * information that needs to be appended to all data packets.
 */
static inline sfp_header_t * csp_sfp_header_add(csp_packet_t * packet) {

	sfp_header_t * header = (sfp_header_t *)&packet->data[packet->length];
	packet->length += sizeof(*header);
	return header;
}

static inline sfp_header_t * csp_sfp_header_remove(csp_packet_t * packet) {

	if ((packet->id.flags & CSP_FFRAG) == 0) {
		return NULL;
	}
	sfp_header_t * header;
	if (packet->length < sizeof(*header)) {
		return NULL;
	}
	header = (sfp_header_t *)&packet->data[packet->length - sizeof(*header)];
	packet->length -= sizeof(*header);

	header->offset = be32toh(header->offset);
	header->totalsize = be32toh(header->totalsize);

	if (header->offset > header->totalsize) {
		return NULL;
	}

	return header;
}

int csp_sfp_send(csp_conn_t * conn, const csp_sfp_read_t * user, uint32_t totalsize, uint32_t mtu) {
	if ((mtu == 0) || (mtu > (CSP_BUFFER_SIZE - sizeof(sfp_header_t)))) {
		return CSP_ERR_INVAL;
	}

	uint32_t count = 0;
	while (count < totalsize) {

		sfp_header_t * sfp_header;

		/* Allocate packet */
		csp_packet_t * packet = csp_buffer_get(0);
		if (packet == NULL) {
			return CSP_ERR_NOMEM;
		}

		/* Calculate sending size */
		uint16_t size = totalsize - count;
		if (size > mtu) {
			size = mtu;
		}

		/* Copy data */
		if (size != user->read(user->data, packet->data, count, size)) {
			csp_buffer_free(packet);
			return CSP_ERR_SFP;
		}
		packet->length = size;

		/* Set fragment flag */
		conn->idout.flags |= CSP_FFRAG;

		/* Add SFP header */
		sfp_header = csp_sfp_header_add(packet);  // no check, because buffer was allocated with extra size.
		sfp_header->totalsize = htobe32(totalsize);
		sfp_header->offset = htobe32(count);

		/* Send data */
		csp_send(conn, packet);

		/* Increment count */
		count += size;
	}

	return CSP_ERR_NONE;
}

int csp_sfp_recv_fp(csp_conn_t * conn, const csp_sfp_write_t * user, uint32_t * return_datasize, uint32_t timeout, csp_packet_t * first_packet) {

	*return_datasize = 0;

	/* Get first packet from user, or from connection */
	csp_packet_t * packet;
	if (first_packet == NULL) {
		packet = csp_read(conn, timeout);
		if (packet == NULL) {
			return CSP_ERR_TIMEDOUT;
		}
	} else {
		packet = first_packet;
	}

	uint32_t datasize = 0;
	uint32_t data_offset = 0;
	int error = CSP_ERR_TIMEDOUT;
	do {
		/* Read SFP header */
		sfp_header_t * sfp_header = csp_sfp_header_remove(packet);
		if (sfp_header == NULL) {
			//csp_print("%s: %u:%u, invalid message, id.flags: 0x%x, length: %u\n", __FUNCTION__, packet->id.src, packet->id.sport, packet->id.flags, packet->length);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		//csp_print("%s: %u:%u, fragment %" PRIu32 "/%" PRIu32 "\n",  __FUNCTION__, packet->id.src, packet->id.sport, sfp_header->offset + packet->length, sfp_header->totalsize);

		/* Consistency check */
		if ((sfp_header->offset != data_offset) || (packet->length == 0) || (packet->length > (sizeof(packet->data) - sizeof(* sfp_header)))) {
			//csp_print("%s: %u:%u, invalid message, offset %" PRIu32 " (expected %" PRIu32 "), length: %u, totalsize %" PRIu32 "\n", __FUNCTION__, packet->id.src, packet->id.sport, sfp_header->offset, data_offset, packet->length, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		/* Set total expected size */
		if (datasize == 0) {
			datasize = sfp_header->totalsize;
			if (datasize == 0) {
				csp_buffer_free(packet);

				error = CSP_ERR_SFP;
				goto error;
			}
		}

		/* Copy data to output */
		if (packet->length != user->write(user->data, packet->data, data_offset, packet->length, sfp_header->totalsize)) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __FUNCTION__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		data_offset += packet->length;

		if (data_offset >= datasize) {
			// transfer complete
			csp_buffer_free(packet);

			*return_datasize = datasize;
			return CSP_ERR_NONE;
		}

		/* Consistency check */
		if (packet->length == 0) {
			//csp_print("%s: %u:%u, invalid size, sfp.offset: %" PRIu32 ", length: %u, total: %" PRIu32 " / %" PRIu32 "\n", __FUNCTION__, packet->id.src, packet->id.sport, sfp_header->offset, packet->length, datasize, sfp_header->totalsize);
			csp_buffer_free(packet);

			error = CSP_ERR_SFP;
			goto error;
		}

		csp_buffer_free(packet);

	} while ((packet = csp_read(conn, timeout)) != NULL);

error:
	return error;
}
