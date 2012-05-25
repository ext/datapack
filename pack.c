#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define CHUNK 16384
static unsigned char  in[CHUNK];
static unsigned char out[CHUNK];

int main(int argc, char* argv[]){
	int ret;
	z_stream strm;
	int flush;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if ( argc < 2 ){
		fprintf(stderr, "usage: c DATANAME:FILENAME..\n");
		return 1;
	}

	fprintf(stdout, "#include \"offsets.h\"\n");

	for ( int set = 1; set < argc; set++ ){
		char* dataname = argv[set];
		char* delim = strchr(dataname, ':');
		if ( !delim ){
			fprintf(stderr, "%s: missing delimiter in `%s', ignored.\n", argv[0], dataname);
			continue;
		}
		*delim = 0;
		char* filename = delim+1;

		FILE* fp = fopen(filename, "r");
		if ( !fp ){
			fprintf(stderr, "%s: failed to read `%s', ignored.\n", argv[0], filename);
			continue;
		}

		fprintf(stdout, "static const char %s_buf[] = {", dataname);

		ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		if (ret != Z_OK)
			return ret;

		size_t input = 0;
		size_t bytes = 0;
		do {
			strm.avail_in = fread(in, 1, CHUNK, fp);
			input += strm.avail_in;
			if (ferror(fp)) {
				deflateEnd(&strm);
				return Z_ERRNO;
			}
			flush = feof(fp) ? Z_FINISH : Z_NO_FLUSH;
			strm.next_in = in;

			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				deflate(&strm, flush);

				unsigned int have = CHUNK - strm.avail_out;
				for ( int i = 0; i < have; i++ ){
					fprintf(stdout, "%s%d", bytes==0?"":",", out[i]);
					bytes++;
				}

			} while (strm.avail_out == 0);
		} while (flush != Z_FINISH);

		fprintf(stdout, "};\n");
		fprintf(stdout, "struct pack_offset %s = {%s_buf, %zd, %zd};\n", dataname, dataname, bytes, input);

		deflateEnd(&strm);
	}

	return Z_OK;
}
