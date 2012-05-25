#ifndef OFFSETS_H
#define OFFSETS_H

#include <stdint.h>
#include <stddef.h>

struct pack_offset {
	const char* data; /* compressed data */
	size_t  in_bytes; /* compressed size */
	size_t out_bytes; /* uncompressed size */
};

extern struct pack_offset DATA1;
extern struct pack_offset DATA2;
extern struct pack_offset DATA3;

int unpack(const struct pack_offset src, char** dst);

#endif /* OFFSETS_H */
