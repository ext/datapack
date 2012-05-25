#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "offsets.h"

#define CHUNK 16384

int unpack(const struct pack_offset src, char** dstptr){
	const size_t bufsize = src.out_bytes;
	char* dst = malloc(bufsize+1); /* must fit null-terminator */

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = src.in_bytes;
	strm.next_in = (void*)src.data;
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

