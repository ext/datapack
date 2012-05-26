#ifndef DATAPACK_H
#define DATAPACK_H

#include <stdint.h>
#include <stddef.h>

struct datapack_file_entry {
	const char filename[64];   /* filename (null-terminated) */
	const char* data;          /* compressed data */
	size_t  in_bytes;          /* compressed size */
	size_t out_bytes;          /* uncompressed size */
};

int unpack(const struct datapack_file_entry src, char** dst);

#endif /* DATAPACK_H */
