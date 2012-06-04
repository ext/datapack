#ifndef DATAPACK_H
#define DATAPACK_H

#include <stdio.h>
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
 * Open packed file as stream.
 *
 * For writing to work override must be enabled and user must have write
 * permission to the directory. If override is disabled it return EPERM.
 * In addition, even for writing it requires that the file already exists as an
 * entry. It cannot be used to create arbitrary files.
 *
 * @return FILE pointer or NULL on errors and errno is set to indicate the error.
 * @error ENOENT if the file does not exists, whenever mode is read or write.
 */
FILE* unpack_open(const char* filename, const char* mode);

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
