#ifndef DATAPACK_H
#define DATAPACK_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datapack* datapack_t;

struct datapack_entry {
	datapack_t handle;         /* which pack this entry belongs to */
	const char* filename;      /* filename (null-terminated) */
	const char* data;          /* compressed data (NULL if data must be read from file first) */
	long offset;               /* offset to data in file (0 if compressed data is present in data) */
	size_t csize;              /* compressed size */
	size_t usize;              /* uncompressed size */
};

/**
 * Opens a new pack.
 *
 * @param filename Filename or NULL for reading in-process data.
 * @return Handle to datapack.
 */
datapack_t datapack_open(const char* filename);

/**
 * Closes an open pack.
 */
void datapack_close(datapack_t handle);

/**
 * Unpack a file using file entry directly.
 * Allocated memory should be freed using free(3).
 */
int unpack(const struct datapack_entry* src, char** dst);

/**
 * Unpack a file using path.
 * Allocated memory should be freed using free(3).
 */
int unpack_filename(datapack_t handle, const char* filename, char** dst);

/**
 * Find a file using path
 */
struct datapack_entry* unpack_find(datapack_t handle, const char* filename);

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
FILE* unpack_open(datapack_t handle, const char* filename, const char* mode);

/**
 * Allow user to override files by placing them in dir using the destination
 * filename used when data was packed.
 *
 * If dir is NULL it disables overriding (default).
 */
int unpack_override(const char* dir);

#ifdef __cplusplus
}
#endif

#endif /* DATAPACK_H */
