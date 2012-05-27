#ifndef DATAPACK_H
#define DATAPACK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct datapack_file_entry {
	const char* filename;      /* filename (null-terminated) */
	const char* data;          /* compressed data */
	size_t  in_bytes;          /* compressed size */
	size_t out_bytes;          /* uncompressed size */
};

int unpack(const struct datapack_file_entry* src, char** dst);
int unpack_filename(const char* filename, char** dst);

#ifdef _cplusplus
}
#endif

#endif /* DATAPACK_H */
