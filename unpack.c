#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "datapack.h"

#define CHUNK 16384

static char* local = NULL;

int unpack_override(const char* dir){
	free((char*)local);

	size_t len = strlen(dir);
	if ( dir[len-1] == '/' ) len--;
	local = strndup(dir, len);

	return 0;
}

int unpack(const struct datapack_file_entry* src, char** dstptr){
	if ( local ){
		char* local_path;
		if ( asprintf(&local_path, "%s/%s", local, src->filename) == -1 ){
			return errno;
		}

		FILE* fp = fopen(local_path, "r");
		free(local_path);

		if ( fp ){
			fseek(fp, 0, SEEK_END);
			const long size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* dst = (char*) malloc((size_t)(size+1)); /* must fit null-terminator */
			if ( fread(dst, (size_t)size, 1, fp) == 0 ){
				return errno;
			}

			dst[size] = 0; /* force null-terminator */
			*dstptr = dst; /* return pointer to caller */
			fclose(fp);

			return 0;
		}
	}

	*dstptr = NULL;
	const size_t bufsize = src->usize;
	char* dst = (char*) malloc(bufsize+1); /* must fit null-terminator */

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = (unsigned int) src->csize;
	strm.next_in = (Bytef*)src->data;
	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	strm.avail_out = (unsigned int) bufsize;
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

struct datapack_file_entry* unpack_find(const char* filename){
	extern struct datapack_file_entry* filetable[];
	struct datapack_file_entry* cur = filetable[0];

	int i = 0;
	while ( cur ){
		if ( strcmp(filename, cur->filename) == 0 ){
			return cur;
		}
		cur = filetable[++i];
	}

	return NULL;
}

int unpack_filename(const char* filename, char** dst){
	*dst = NULL;

	struct datapack_file_entry* entry = unpack_find(filename);
	if ( !entry ){
		return ENOENT;
	}

	return unpack(entry, dst);
}

/**
 * Recursive mkdir.
 */
static void rec_mkdir(const char *path){
	char* tmp = strdup(path);
	const size_t len = strlen(tmp);
	if ( tmp[len-1] == '/' ) tmp[len-1] = 0;

	for ( char* p = tmp; *p; p++ ){
		if ( *p != '/') continue;

		*p = 0;
		if ( access(tmp, F_OK) ){
			mkdir(tmp, S_IRWXU);
		}
		*p = '/';
	}

	if ( access(tmp, F_OK) ){
		mkdir(tmp, S_IRWXU);
	}

	free(tmp);
}

struct unpack_cookie_data {
	const struct datapack_file_entry* src;
	z_stream strm;
	size_t bufsize;
	unsigned char buffer[CHUNK];
};

static ssize_t unpack_read(void* cookie, char* buf, size_t size){
	struct unpack_cookie_data* ctx = (struct unpack_cookie_data*)cookie;

	const size_t left = CHUNK - ctx->bufsize;

	if ( ctx->strm.avail_in > 0 ){
		ctx->strm.avail_out = (unsigned int) left;
		ctx->strm.next_out  = &ctx->buffer[ctx->bufsize];

		int ret = inflate(&ctx->strm, Z_NO_FLUSH);
		switch (ret) {
		case Z_NEED_DICT:
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			inflateEnd(&ctx->strm);
			errno = EIO;
			return -1;
		}

		ctx->bufsize += left - ctx->strm.avail_out;
	}

	const size_t bytes = size < ctx->bufsize ? size : ctx->bufsize;

	memcpy(buf, ctx->buffer, bytes);
	memmove(ctx->buffer, &ctx->buffer[size], bytes);
	ctx->bufsize -= bytes;

	return (ssize_t) bytes;
}

static int unpack_close(void *cookie){
	struct unpack_cookie_data* ctx = (struct unpack_cookie_data*)cookie;

	inflateEnd(&ctx->strm);
	free(ctx);

	return 0;
}

static cookie_io_functions_t unpack_cookie_func = {
	unpack_read,
	NULL,
	NULL,
	unpack_close
};

FILE* unpack_open(const char* filename, const char* mode){
	struct datapack_file_entry* entry = unpack_find(filename);
	if ( !entry ){
		errno = ENOENT;
		return NULL;
	}

	const int write = strchr(mode, 'w') || strchr(mode, 'a');
	const int read = !write;

	/* allow overriding with local path */
	if ( read && local ){
		char* local_path;
		if(asprintf(&local_path, "%s/%s", local, filename) == -1) return NULL;
		FILE* fp = fopen(local_path, mode);
		free(local_path);
		if ( fp ){
			return fp;
		}
	}

	if ( write ) {
		/* ensure write is done in local mode */
		if ( !local ){
			errno = EPERM;
			return NULL;
		}

		/* ensure directory exists */
		rec_mkdir(local);

		/* give file pointer directly from fopen */
		char* local_path;
		if(asprintf(&local_path, "%s/%s", local, filename) == -1) return NULL;
		FILE* fp = fopen(local_path, mode);
		free(local_path);

		return fp;
	}

	struct unpack_cookie_data* ctx = (struct unpack_cookie_data*)malloc(sizeof(struct unpack_cookie_data));
	ctx->src = entry;
	ctx->bufsize = 0;

	ctx->strm.avail_in = (unsigned int) entry->csize;
	ctx->strm.next_in = (unsigned char*)entry->data;
	ctx->strm.zalloc = Z_NULL;
	ctx->strm.zfree = Z_NULL;
	ctx->strm.opaque = Z_NULL;
	int ret = inflateInit(&ctx->strm);
	if (ret != Z_OK){
		errno = EBADFD;
		return NULL;
	}

	return fopencookie(ctx, mode, unpack_cookie_func);
}
