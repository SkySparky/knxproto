#ifndef HPKNXD_MESSAGEBUILDER_H
#define HPKNXD_MESSAGEBUILDER_H

#include "alloc.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Message Builder
 */
typedef struct {
	uint8_t* buffer;
	size_t used, max;
} msgbuilder;

/**
 * Allocate and initialize.
 */
msgbuilder* msgbuilder_new(size_t cap);

/**
 * Initialize and allocate buffer with the given capacity `cap`.
 */
bool msgbuilder_init(msgbuilder* mb, size_t cap);

/**
 * Free the underlying buffer.
 */
inline void msgbuilder_reset(msgbuilder* mb) {
	if (mb->buffer) {
		free(mb->buffer);
		mb->buffer = NULL;
	}

	mb->used = mb->max = 0;
}

/**
 * Free the entire structure including the buffer.
 */
inline void msgbuilder_free(msgbuilder* mb) {
	if (mb->buffer)
		free(mb->buffer);

	free(mb);
}

/**
 * Ensure space for `num` more elements.
 */
bool msgbuilder_reserve(msgbuilder* mb, size_t num);

/**
 * Append data.
 */
bool msgbuilder_append(msgbuilder* mb,
                       const uint8_t* restrict source,
                       size_t length);

/**
 * Append data from another `msgbuilder`.
 */
bool msgbuilder_append_mb(const msgbuilder* mb, const msgbuilder* rhs);


#endif
