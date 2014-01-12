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
#include <dlfcn.h>
#include "datapack.h"
#include "pak.h"

#define CHUNK 16384

static char* local = NULL;

struct datapack {
	FILE* fp;
	size_t num_entries;
	void (*cleanup)(datapack_t handle);
	char** filename;
	struct datapack_entry* filetable[];
};

static void datapack_proc_cleanup(datapack_t handle){
	/* nothing to cleanup */
}

static datapack_t datapack_open_proc(){
	void* dl = dlopen(NULL, RTLD_LAZY);
	if ( !dl ){
		errno = EINVAL;
		return NULL;
	}

	const struct datapack_entry** filetable = (const struct datapack_entry**)dlsym(dl, "filetable");
	if ( !filetable ){
		errno = ENOENT;
		return NULL;
	}

	size_t n = 1; /* 1 slot for sentinel */
	const struct datapack_entry* cur = filetable[0];
	while ( cur ){
		cur = filetable[++n];
	}

	const size_t tablesize = sizeof(struct datapack_entry) * n;
	datapack_t pak = (datapack_t)malloc(sizeof(struct datapack) + tablesize);
	pak->fp = NULL;
	pak->num_entries = n;
	pak->filename = NULL;
	pak->cleanup = datapack_proc_cleanup;
	memcpy(pak->filetable, filetable, tablesize);
	/** @todo fill handle */

	return pak;
}

static void datapack_file_cleanup(datapack_t handle){
	for ( int i = 0; i < handle->num_entries; i++ ){
		free(handle->filename[i]);
		free(handle->filetable[i]);
	}

	fclose(handle->fp);
	free(handle->filename);
}

datapack_t datapack_open(const char* filename){
	if ( !filename ){
		return datapack_open_proc();
	}

	FILE* fp = fopen(filename, "r");
	if ( !fp ) return NULL;

	static unsigned char expected[] = DATAPACK_MAGIC;
	static unsigned char actual[sizeof(expected)];
	if ( fread(actual, sizeof(expected), 1, fp) != 1 ){
		return NULL;
	}
	if ( memcmp(expected, actual, sizeof(expected)) != 0 ){
		errno = EINVAL;
		return NULL;
	}

	/* read and validate header */
	struct datapack_pak_header header;
	if ( fread(&header, sizeof(struct datapack_pak_header), 1, fp) != 1 ){
		return NULL;
	}
	if ( header.dp_version != 1 ){
		errno = EINVAL;
		return NULL;
	}

	/* parse header */
	long offset = be16toh(header.dp_offset);
	size_t num_entries = (size_t)be16toh(header.dp_num_entries);
	fseek(fp, offset, SEEK_SET);

	/* allocate new table (native format) */
	const size_t tablesize = sizeof(struct datapack_entry) * (num_entries + 1); /* +1 for sentinel */
	datapack_t pak = (datapack_t)malloc(sizeof(struct datapack) + tablesize);
	pak->fp = fp;
	pak->num_entries = num_entries;
	pak->filename = (char**)malloc(sizeof(char*) * num_entries);
	pak->cleanup = datapack_file_cleanup;
	memset(&pak->filetable, 0, tablesize);

	for ( unsigned int i = 0; i < num_entries; i++ ){
		/* read entry */
		struct datapack_pakfile_entry packed;
		if ( fread(&packed, sizeof(struct datapack_pakfile_entry), 1, fp) != 1 ){
			errno = EBADF;
			return NULL;
		}
		const size_t csize = be32toh(packed.csize);
		const size_t usize = be32toh(packed.usize);
		const size_t fsize = be32toh(packed.fsize);

		/* read filename */
		char* filename = (char*)malloc(fsize+1); /* +1 for null-terminator */
		if ( fread(filename, fsize, 1, fp) != 1 ){
			errno = EBADF;
			return NULL;
		}
		filename[fsize] = 0;

		/* store entry */
		struct datapack_entry* entry = malloc(sizeof(struct datapack_entry));
		entry->handle = pak;
		entry->filename = filename;
		entry->data = NULL;
		entry->offset = ftell(fp);
		entry->csize = csize;
		entry->usize = usize;
		pak->filetable[i] = entry;
		pak->filename[i] = filename;

		/* skip data */
		fseek(fp, (long)csize, SEEK_CUR);
	}

	return pak;
}

void datapack_close(datapack_t handle){
	handle->cleanup(handle);
	free(handle);
}

int unpack_override(const char* dir){
	free((char*)local);

	size_t len = strlen(dir);
	if ( dir[len-1] == '/' ) len--;
	local = strndup(dir, len);

	return 0;
}

int unpack(const struct datapack_entry* src, char** dstptr){
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

	/* prepare destination buffer */
	*dstptr = NULL;
	const size_t bufsize = src->usize;
	char* dst = (char*) malloc(bufsize+1); /* must fit null-terminator */

	/* prepare source buffer */
	unsigned char* srcbuf = (unsigned char*)malloc(src->csize);
	if ( src->data ){
		memcpy(srcbuf, src->data, src->csize);
	} else {
		fseek(src->handle->fp, src->offset, SEEK_SET);
		if ( fread(srcbuf, src->csize, 1, src->handle->fp) != 1 ){
			/* @todo read in chunks */
			return EBADF;
		}
	}

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = (unsigned int) src->csize;
	strm.next_in = (Bytef*)srcbuf;
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
	free(srcbuf);
	inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

struct datapack_entry* unpack_find(datapack_t handle, const char* filename){
	if ( !handle ) return NULL;
	struct datapack_entry* cur = handle->filetable[0];

	int i = 0;
	while ( cur ){
		if ( strcmp(filename, cur->filename) == 0 ){
			return cur;
		}
		cur = handle->filetable[++i];
	}

	return NULL;
}

int unpack_filename(datapack_t handle, const char* filename, char** dst){
	*dst = NULL;

	struct datapack_entry* entry = unpack_find(handle, filename);
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
	const struct datapack_entry* src;
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

FILE* unpack_open(datapack_t handle, const char* filename, const char* mode){
	struct datapack_entry* entry = unpack_find(handle, filename);
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

const char* datapack_version(datapack_version_t* version){
	if ( version ){
		version->major = VERSION_MAJOR;
		version->minor = VERSION_MINOR;
		version->micro = VERSION_MICRO;
	}
	return VERSION;
}
