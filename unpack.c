#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include "datapack.h"

#define CHUNK 16384

static const char* local = NULL;

int unpack_override(const char* dir){
	local = dir;
	return 0;
}

int unpack(const struct datapack_file_entry* src, char** dstptr){
	if ( local ){
		char* local_path;
		asprintf(&local_path, "%s%s", local, src->filename);

		FILE* fp = fopen(local_path, "r");
		free(local_path);

		if ( fp ){
			fseek(fp, 0, SEEK_END);
			const long size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* dst = malloc(size+1); /* must fit null-terminator */
			if ( fread(dst, size, 1, fp) == 0 ){
				return errno;
			}

			dst[size] = 0; /* force null-terminator */
			*dstptr = dst; /* return pointer to caller */
			fclose(fp);

			return 0;
		}
	}

	*dstptr = NULL;
	const size_t bufsize = src->out_bytes;
	char* dst = malloc(bufsize+1); /* must fit null-terminator */

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = src->in_bytes;
	strm.next_in = (void*)src->data;
	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	strm.avail_out = bufsize;
	strm.next_out = (unsigned char*)dst;
	ret = inflate(&strm, Z_NO_FLUSH);
	switch (ret) {
	case Z_NEED_DICT:
		ret = Z_DATA_ERROR;
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
		inflateEnd(&strm);
		return ret;
	}

	dst[bufsize-strm.avail_out] = 0; /* force null-terminator */
	*dstptr = dst;                   /* return pointer to caller */

	/* clean up and return */
	inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int unpack_filename(const char* filename, char** dst){
	*dst = NULL;
	extern struct datapack_file_entry* filetable[];

	int i = 0;
	struct datapack_file_entry* cur = filetable[0];
	while ( cur ){
		if ( strcmp(filename, cur->filename) == 0 ){
			return unpack(cur, dst);
		}
		cur = filetable[++i];
	}

	return ENOENT;
}
