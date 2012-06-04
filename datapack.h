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

/**
 * Unpack a file using file entry directly.
 * Allocated memory should be freed using free(3).
 */
int unpack(const struct datapack_file_entry* src, char** dst);

/**
 * Unpack a file using path.
 * Allocated memory should be freed using free(3).
 */
int unpack_filename(const char* filename, char** dst);

/**
 * Allow user to override files by placing them in dir using the destination
 * filename used when data was packed.
 *
 * If dir is NULL it disables overriding (default).
 */
int unpack_override(const char* dir);

#ifdef _cplusplus
}
#endif

#endif /* DATAPACK_H */
